#!/usr/bin/env python3
"""Convert GitHub Issues to traceability.json format

Fetches requirements from GitHub Issues and generates the same traceability.json
format that build_trace_json.py produces from markdown specs.

This ensures compatibility with validate-trace-coverage.py and other tools
that expect the build/traceability.json format.

Output format:
{
    "metrics": {
        "requirement": {"coverage_pct": 82.0, "total": 50, "linked": 41},
        "requirement_to_ADR": {"coverage_pct": 75.0},
        "requirement_to_scenario": {"coverage_pct": 60.0},
        "requirement_to_test": {"coverage_pct": 40.0}
    },
    "items": [...],
    "forward_links": {...},
    "backward_links": {...}
}
"""
import os
import sys
import json
import re
from pathlib import Path
from collections import defaultdict
from github import Github

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build' / 'traceability.json'

def extract_issue_links(body: str) -> dict:
    """Extract traceability links from issue body."""
    if not body:
        return {}
    
    links = defaultdict(list)
    
    # Match various link patterns
    patterns = {
        'traces_to': r'(?:Traces to|Parent|Traces-to):\s*#(\d+)',
        'depends_on': r'(?:Depends on|Depends-on):\s*#(\d+)',
        'verified_by': r'(?:Verified by|Test|Verified-by):\s*#(\d+)',
        'implemented_by': r'(?:Implemented by|Implements|Implemented-by):\s*#(\d+)',
    }
    
    for link_type, pattern in patterns.items():
        matches = re.findall(pattern, body, re.IGNORECASE)
        links[link_type].extend(int(m) for m in matches)
    
    return dict(links)

def get_requirement_type(title: str, labels: list) -> str:
    """Determine requirement type from title and labels."""
    # Extract from title prefix
    match = re.match(r'^(StR|REQ-F|REQ-NF|ADR|ARC-C|QA-SC|TEST)', title)
    if match:
        return match.group(1)
    
    # Fallback to labels
    label_map = {
        'type:stakeholder-requirement': 'StR',
        'type:requirement:functional': 'REQ-F',
        'type:requirement:non-functional': 'REQ-NF',
        'type:architecture:decision': 'ADR',
        'type:architecture:component': 'ARC-C',
        'type:architecture:quality-scenario': 'QA-SC',
        'type:test-case': 'TEST',
    }
    
    for label in labels:
        if label in label_map:
            return label_map[label]
    
    return 'UNKNOWN'

def main() -> int:
    token = os.environ.get('GITHUB_TOKEN')
    if not token:
        print('ERROR: GITHUB_TOKEN environment variable required', file=sys.stderr)
        return 1
    
    repo_name = os.environ.get('GITHUB_REPOSITORY', 'zarfld/ESP_ClapMetronome')
    
    try:
        g = Github(token)
        repo = g.get_repo(repo_name)
    except Exception as e:
        print(f'ERROR: Failed to connect to GitHub: {e}', file=sys.stderr)
        return 1
    
    print(f"Fetching issues from {repo_name}...")
    
    # Fetch all requirement issues
    requirement_labels = [
        'type:stakeholder-requirement',
        'type:requirement:functional',
        'type:requirement:non-functional',
        'type:architecture:decision',
        'type:architecture:component',
        'type:architecture:quality-scenario',
        'type:test-case',
    ]
    
    all_issues = []
    for label in requirement_labels:
        try:
            issues = list(repo.get_issues(labels=[label], state='all'))
            all_issues.extend(issues)
        except Exception as e:
            print(f"Warning: Could not fetch label {label}: {e}", file=sys.stderr)
    
    if not all_issues:
        print("Warning: No issues found with requirement labels", file=sys.stderr)
    
    print(f"Found {len(all_issues)} requirement issues")
    
    # Build traceability structure
    items = []
    forward_links = {}
    backward_links = defaultdict(list)
    
    # Track requirements by type for metrics
    requirements = []  # REQ-F, REQ-NF
    requirements_with_adr = set()
    requirements_with_scenario = set()
    requirements_with_test = set()
    requirements_with_any_link = set()
    
    for issue in all_issues:
        issue_id = f"#{issue.number}"
        labels = [l.name for l in issue.labels]
        req_type = get_requirement_type(issue.title, labels)
        
        links = extract_issue_links(issue.body or "")
        
        # Build item entry
        item = {
            'id': issue_id,
            'type': req_type,
            'title': issue.title,
            'state': issue.state,
            'url': issue.html_url,
            'references': []
        }
        
        # Collect all referenced issues
        all_refs = set()
        for link_list in links.values():
            all_refs.update(f"#{n}" for n in link_list)
        item['references'] = sorted(all_refs)
        
        items.append(item)
        forward_links[issue_id] = item['references']
        
        # Build backward links
        for ref in item['references']:
            backward_links[ref].append(issue_id)
        
        # Track metrics for requirements
        if req_type in ['REQ-F', 'REQ-NF']:
            requirements.append(issue_id)
            
            if item['references']:
                requirements_with_any_link.add(issue_id)
            
            # Check what this requirement links to
            for ref_num in links.get('traces_to', []):
                # Fetch the referenced issue to check its type
                try:
                    ref_issue = repo.get_issue(ref_num)
                    ref_labels = [l.name for l in ref_issue.labels]
                    ref_type = get_requirement_type(ref_issue.title, ref_labels)
                    
                    if ref_type == 'ADR':
                        requirements_with_adr.add(issue_id)
                    elif ref_type == 'QA-SC':
                        requirements_with_scenario.add(issue_id)
                except:
                    pass
            
            if links.get('verified_by'):
                requirements_with_test.add(issue_id)
    
    # Calculate metrics
    total_reqs = len(requirements)
    metrics = {
        'requirement': {
            'coverage_pct': (len(requirements_with_any_link) / total_reqs * 100) if total_reqs else 0,
            'total': total_reqs,
            'linked': len(requirements_with_any_link)
        }
    }
    
    if total_reqs > 0:
        metrics['requirement_to_ADR'] = {
            'coverage_pct': len(requirements_with_adr) / total_reqs * 100,
            'total': total_reqs,
            'linked': len(requirements_with_adr)
        }
        metrics['requirement_to_scenario'] = {
            'coverage_pct': len(requirements_with_scenario) / total_reqs * 100,
            'total': total_reqs,
            'linked': len(requirements_with_scenario)
        }
        metrics['requirement_to_test'] = {
            'coverage_pct': len(requirements_with_test) / total_reqs * 100,
            'total': total_reqs,
            'linked': len(requirements_with_test)
        }
    
    # Build output
    output = {
        'source': 'github-issues',
        'repository': repo_name,
        'generated_at': __import__('datetime').datetime.utcnow().isoformat(),
        'metrics': metrics,
        'items': items,
        'forward_links': forward_links,
        'backward_links': {k: list(v) for k, v in backward_links.items()}
    }
    
    # Write output
    OUT.parent.mkdir(exist_ok=True)
    OUT.write_text(json.dumps(output, indent=2), encoding='utf-8')
    
    print(f"\n✅ Generated {OUT}")
    print(f"   Total items: {len(items)}")
    print(f"   Requirements: {total_reqs}")
    if total_reqs > 0:
        print(f"   Overall coverage: {metrics['requirement']['coverage_pct']:.1f}%")
        print(f"   ADR linkage: {metrics.get('requirement_to_ADR', {}).get('coverage_pct', 0):.1f}%")
        print(f"   Scenario linkage: {metrics.get('requirement_to_scenario', {}).get('coverage_pct', 0):.1f}%")
        print(f"   Test linkage: {metrics.get('requirement_to_test', {}).get('coverage_pct', 0):.1f}%")
    
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
