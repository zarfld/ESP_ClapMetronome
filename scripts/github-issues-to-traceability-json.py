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
    """Extract traceability links from issue body.
    
    Handles multiple formats:
    1. Inline bold: **Traces to**: #123
    2. Bold with markdown: **Parent**: #1 (StR-001: Description)
    3. Section headers with lists:
       ## Traceability
       **Implements Requirements**:
       - #2 (REQ-F-001: Description)
    4. Narrative format: **Addresses Requirements**: #2, #6, #7
    """
    if not body:
        return {}
    
    links = defaultdict(list)
    
    # Pattern 1: Bold inline format (with or without markdown **)
    # Matches: **Traces to**: #123 or Traces to: #123 or **Parent**: #1 (Description)
    patterns = {
        'traces_to': [
            r'\*\*(?:Traces?\s+to|Parent|Traces-to)\*\*:\s*#(\d+)',  # **Traces to**: #N
            r'(?:^|\n)(?:Traces?\s+to|Parent|Traces-to):\s*#(\d+)',  # Traces to: #N (no bold)
        ],
        'depends_on': [
            r'\*\*(?:Depends?\s+on|Depends-on)\*\*:\s*#(\d+)',
            r'(?:^|\n)(?:Depends?\s+on|Depends-on):\s*#(\d+)',
        ],
        'verified_by': [
            r'\*\*(?:Verified\s+by|Test|Verified-by|Verifies\s+Requirements?)\*\*:\s*#(\d+)',
            r'(?:^|\n)(?:Verified\s+by|Test|Verified-by|Verifies\s+Requirements?):\s*#(\d+)',
        ],
        'implemented_by': [
            r'\*\*(?:Implemented\s+by|Implements?|Implemented-by)\*\*:\s*#(\d+)',
            r'(?:^|\n)(?:Implemented\s+by|Implements?|Implemented-by):\s*#(\d+)',
        ],
    }
    
    # Pattern 2: Multi-word section labels with lists
    # Matches: **Implements Requirements**:\n- #2 (REQ-F-001)
    section_patterns = {
        'traces_to': r'\*\*(?:Traces?\s+to|Parent|Satisfies|Addresses)(?:\s+Requirements?)?\*\*:[^#]*?(?:^|\n)\s*-?\s*#(\d+)',
        'depends_on': r'\*\*(?:Depends?\s+on|Dependencies|Required)\*\*:[^#]*?(?:^|\n)\s*-?\s*#(\d+)',
        'verified_by': r'\*\*(?:Verified\s+by|Test|Validates?|Verifies)(?:\s+Requirements?)?\*\*:[^#]*?(?:^|\n)\s*-?\s*#(\d+)',
        'implemented_by': r'\*\*(?:Implemented\s+by|Implements?)(?:\s+Requirements?)?\*\*:[^#]*?(?:^|\n)\s*-?\s*#(\d+)',
    }
    
    # Additional patterns for architecture issues
    architecture_patterns = {
        'traces_to': r'\*\*(?:Addresses|Satisfies)\s+Requirements?\*\*:[^#]*?#(\d+)',  # ADR pattern
        'implemented_by': r'\*\*(?:Components?\s+Affected|Architecture\s+Decisions?)\*\*:[^#]*?#(\d+)',
        'verified_by': r'\*\*(?:Quality\s+Scenarios?|Requirements?\s+Verified)\*\*:[^#]*?#(\d+)',
    }
    
    # Extract all patterns
    for link_type, pattern_list in patterns.items():
        for pattern in pattern_list:
            matches = re.findall(pattern, body, re.IGNORECASE | re.MULTILINE)
            links[link_type].extend(int(m) for m in matches)
    
    for link_type, pattern in section_patterns.items():
        matches = re.findall(pattern, body, re.IGNORECASE | re.MULTILINE | re.DOTALL)
        links[link_type].extend(int(m) for m in matches)
    
    for link_type, pattern in architecture_patterns.items():
        matches = re.findall(pattern, body, re.IGNORECASE | re.MULTILINE | re.DOTALL)
        links[link_type].extend(int(m) for m in matches)
    
    # Generic pattern: find all issue references in traceability sections
    # Look for ## Traceability or ## Traces To sections and extract all #N references
    traceability_sections = re.findall(
        r'##\s+(?:Traceability|Traces\s+To).*?(?=##|$)',
        body,
        re.IGNORECASE | re.MULTILINE | re.DOTALL
    )
    
    for section in traceability_sections:
        # Extract all #N references from the section
        all_refs = re.findall(r'#(\d+)', section)
        # Add to traces_to if not already captured
        for ref in all_refs:
            ref_int = int(ref)
            if ref_int not in links['traces_to']:
                links['traces_to'].append(ref_int)
    
    # Remove duplicates while preserving order
    for key in links:
        links[key] = list(dict.fromkeys(links[key]))
    
    return dict(links)

def get_requirement_type(title: str, labels: list) -> str:
    """Determine requirement type from title and labels.
    
    Prioritizes title prefix, then checks labels (including colon-separated variants).
    """
    # Extract from title prefix (most reliable)
    match = re.match(r'^(StR|REQ-F|REQ-NF|ADR|ARC-C|QA-SC|TEST)', title, re.IGNORECASE)
    if match:
        return match.group(1).upper()
    
    # Fallback to labels (handle both hyphen and colon separators)
    label_map = {
        # Colon-separated (current project standard)
        'type:stakeholder-requirement': 'StR',
        'type:requirement:functional': 'REQ-F',
        'type:requirement:non-functional': 'REQ-NF',
        'type:architecture:decision': 'ADR',
        'type:architecture:component': 'ARC-C',
        'type:architecture:quality-scenario': 'QA-SC',
        'type:test-case': 'TEST',
        'type:test-plan': 'TEST',
        
        # Hyphen-separated (legacy/alternative)
        'stakeholder-requirement': 'StR',
        'functional-requirement': 'REQ-F',
        'non-functional': 'REQ-NF',
        'architecture-decision': 'ADR',
        'architecture-component': 'ARC-C',
        'quality-scenario': 'QA-SC',
        'test-case': 'TEST',
        'test-plan': 'TEST',
    }
    
    for label in labels:
        # Check exact match first
        if label in label_map:
            return label_map[label]
        
        # Check if label contains any key as substring (partial match)
        label_lower = label.lower()
        if 'stakeholder' in label_lower:
            return 'StR'
        elif 'functional' in label_lower and 'non' not in label_lower:
            return 'REQ-F'
        elif 'non-functional' in label_lower:
            return 'REQ-NF'
        elif 'decision' in label_lower:
            return 'ADR'
        elif 'component' in label_lower:
            return 'ARC-C'
        elif 'quality' in label_lower or 'scenario' in label_lower:
            return 'QA-SC'
        elif 'test' in label_lower:
            return 'TEST'
    
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
        # Primary labels (colon-separated)
        'type:stakeholder-requirement',
        'type:requirement:functional',
        'type:requirement:non-functional',
        'type:architecture:decision',
        'type:architecture:component',
        'type:architecture:quality-scenario',
        'type:test-case',
        'type:test-plan',
        
        # Phase labels (to catch issues tagged by phase)
        'phase:01-stakeholder-requirements',
        'phase:02-requirements',
        'phase:03-architecture',
        'phase:07-verification-validation',
    ]
    
    all_issues = []
    seen_numbers = set()  # Avoid duplicates
    
    for label in requirement_labels:
        try:
            issues = list(repo.get_issues(labels=[label], state='all'))
            for issue in issues:
                if issue.number not in seen_numbers:
                    all_issues.append(issue)
                    seen_numbers.add(issue.number)
        except Exception as e:
            print(f"Warning: Could not fetch label {label}: {e}", file=sys.stderr)
    
    # If no labeled issues found, try fetching all open issues and filter by title prefix
    if not all_issues:
        print("Warning: No issues found with requirement labels, trying title-based detection...", file=sys.stderr)
        try:
            all_open_issues = list(repo.get_issues(state='all'))
            for issue in all_open_issues:
                if re.match(r'^(StR|REQ-F|REQ-NF|ADR|ARC-C|QA-SC|TEST)', issue.title, re.IGNORECASE):
                    if issue.number not in seen_numbers:
                        all_issues.append(issue)
                        seen_numbers.add(issue.number)
        except Exception as e:
            print(f"Warning: Could not fetch all issues: {e}", file=sys.stderr)
    
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
            'labels': labels,  # Include labels for debugging
            'references': [],
            'link_details': {}  # Categorized links for debugging
        }
        
        # Collect all referenced issues
        all_refs = set()
        for link_type, link_list in links.items():
            all_refs.update(f"#{n}" for n in link_list)
            if link_list:
                item['link_details'][link_type] = [f"#{n}" for n in link_list]
        
        item['references'] = sorted(all_refs, key=lambda x: int(x[1:]))  # Sort numerically
        
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
            # Combine all link types for comprehensive tracking
            all_linked_issues = set()
            for link_list in links.values():
                all_linked_issues.update(link_list)
            
            for ref_num in all_linked_issues:
                # Fetch the referenced issue to check its type
                try:
                    ref_issue = repo.get_issue(ref_num)
                    ref_labels = [l.name for l in ref_issue.labels]
                    ref_type = get_requirement_type(ref_issue.title, ref_labels)
                    
                    # Track linkage to ADRs (from any link type)
                    if ref_type == 'ADR':
                        requirements_with_adr.add(issue_id)
                    # Track linkage to Quality Scenarios
                    elif ref_type == 'QA-SC':
                        requirements_with_scenario.add(issue_id)
                    # Also count reverse linkage (ADR/ARC-C linking to this requirement)
                    elif req_type in ['ADR', 'ARC-C'] and ref_type in ['REQ-F', 'REQ-NF']:
                        # This is an architecture artifact linking to a requirement
                        requirements_with_adr.add(f"#{ref_num}")
                except Exception as e:
                    # Don't fail on individual issue fetch errors
                    print(f"Debug: Could not fetch issue #{ref_num}: {e}", file=sys.stderr)
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
