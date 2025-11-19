# CI/CD Setup: GitHub Project & Issues Integration

**Status**: Ready for setup
**Purpose**: Enable automated traceability validation via GitHub Issues
**Date**: 2025-11-19

---

## Overview

The CI workflows are now configured to **dynamically query GitHub Issues** for:
- Requirements traceability validation
- Acceptance criteria status
- Test coverage tracking
- Orphan code detection

This document guides you through the GitHub Project setup and initial issue creation.

---

## Step 1: Create GitHub Project (Manual)

GitHub Projects API v2 requires manual setup via UI.

### 1.1 Create Project

1. Navigate to: <https://github.com/zarfld/ESP_ClapMetronome/projects>
2. Click **"New project"**
3. Select **"Table"** template
4. Set Name: `Requirements Traceability System`
5. Set Description:
   ```
   Standards-compliant requirements tracking (ISO/IEC/IEEE 29148:2018).
   Tracks StR, REQ-F, REQ-NF, ADR, ARC-C, QA-SC, and TEST issues with full bidirectional traceability.
   ```
6. Click **"Create project"**

### 1.2 Custom Fields (Optional - Visual Organization Only)

⚠️ **IMPORTANT**: Project custom fields are **optional** for visual organization. The CI workflows **do NOT use Project fields** - they query data directly from **GitHub Issue bodies** via the Issue Templates (`.github/ISSUE_TEMPLATE/*.yml`).

**Data Flow**:
```
Issue Templates (.github/ISSUE_TEMPLATE/*.yml)
    ↓ User fills form fields
GitHub Issue Body (structured text)
    ↓ CI workflows use PyGithub API
Dynamic Validation (scripts/github-*.py)
```

**If you want visual Project board organization**, you can optionally add these custom fields:

| Field Name | Type | Notes |
|------------|------|-------|
| **Status** | Status | Built-in (Todo/In Progress/Done) |
| **Priority** | Single Select | P0-P3 (also in issue labels) |
| **Phase** | Single Select | Phase 01-09 (also in issue labels) |

**Skip custom fields if**:
- You only need CI validation (labels + issue body text are sufficient)
- You prefer using GitHub labels for filtering
- You want minimal setup

### 1.3 Configure Views

Create these views (click "+" next to view tabs):

#### View 1: Backlog (Board)
- **Layout**: Board
- **Group By**: Status
- **Sort**: Priority → Created Date
- **Purpose**: Kanban workflow

#### View 2: Traceability Matrix (Table)
- **Layout**: Table
- **Columns**: Title, Requirement Type, Priority, Phase, Upstream Link, Downstream Links, Verified, Implemented, Status
- **Sort**: Requirement Type → Phase → Priority
- **Purpose**: Spreadsheet-like traceability view

#### View 3: Test Coverage (Table)
- **Layout**: Table
- **Filter**: Requirement Type = "TEST" OR label = "test-case"
- **Columns**: Title, Upstream Link (verifies), Verified, Priority, Status
- **Purpose**: Track test coverage

### 1.4 Link Repository

1. In project → **"⋮" (Settings)** → **"Manage access"**
2. Click **"Add repository"**
3. Select `ESP_ClapMetronome`
4. Set permissions: **Write** (allows automation to update)
5. Click **"Add repository"**

---

## Step 2: Create Initial GitHub Issues

Use the prepared issue templates in `.github/ISSUE_TEMPLATE/`.

### 2.1 Priority: Create Test Issues for Wave 2.1

The 136 tests we completed need corresponding GitHub Issues for traceability.

**Suggested approach**: Create one TEST issue per TDD cycle:

```bash
# Use GitHub CLI to create issues programmatically
gh issue create --title "TEST-AUDIO-001: Adaptive Threshold Tests" \
  --label "test-case,phase-05,priority-p1" \
  --body "## Test Suite
- **File**: test/test_audio/test_adaptive_threshold.cpp
- **Tests**: 5 tests
- **Status**: ✅ Passing (100%)

## Verified Requirements
Verifies: #<REQ-F issue for AC-AUDIO-001>

## Test Cases
1. Threshold increases with strong signals
2. Threshold decreases in quiet periods
3. Noise floor tracking accuracy
4. Signal envelope detection
5. Boundary conditions

## Execution Results
- All 5 tests passing
- Coverage: 100% of adaptive threshold logic
- Performance: <1ms per test

## Traceability
- **Cycle**: TDD Cycle 1
- **Acceptance Criteria**: AC-AUDIO-001
- **Implementation**: AudioDetection::updateThreshold()
- **Commit**: 9a2e7e4"
```

Repeat for all 13 cycles (skip Cycle 6 - deferred).

### 2.2 Create Requirement Issues

For each acceptance criteria (AC-AUDIO-001 through AC-AUDIO-014):

```bash
gh issue create --title "REQ-F-AUDIO-001: Adaptive Threshold Adjustment" \
  --label "functional-requirement,phase-02,priority-p1" \
  --body "## Description
System shall dynamically adjust detection threshold based on ambient noise level.

## Rationale
Essential for reliable detection across varying acoustic environments.

## Acceptance Criteria
Given varying noise levels
When audio samples are processed
Then threshold adapts to maintain consistent sensitivity

## Traceability
- **Traces to**: #<StR issue - stakeholder need>
- **Verified by**: #<TEST-AUDIO-001 issue>
- **Implemented by**: PR #<implementation PR>

## Priority
P1 (High) - Core detection functionality

## Standards
ISO/IEC/IEEE 29148:2018 - Functional Requirement"
```

### 2.3 Automate Issue Creation (Recommended)

Create a script to generate all issues from TDD cycle documentation:

**File**: `scripts/create-github-issues-from-tdd.py`

```python
#!/usr/bin/env python3
"""
Create GitHub Issues from TDD cycle documentation.

Usage: python scripts/create-github-issues-from-tdd.py --dry-run
       python scripts/create-github-issues-from-tdd.py --execute
"""

import os
import re
from github import Github

# Parse 05-implementation/TDD-CYCLE-*-SUCCESS.md files
# Extract: cycle number, tests count, acceptance criteria
# Create TEST issues programmatically

# See: docs/QUICK-START-github-issues.md for issue body templates
```

---

## Step 3: Update CI Workflows (Already Done ✅)

The following workflows are now configured to query GitHub Issues dynamically:

### 3.1 `ci-standards-compliance.yml`

**Acceptance Tests Job**:
```yaml
- Uses PyGithub to query issues with labels:
  - "test-case"
  - "functional-requirement"
  - "non-functional"
- Validates traceability links in issue bodies
- Reports coverage percentage dynamically
```

**Requirements Traceability Job**:
```yaml
- Runs scripts/github-orphan-check.py
- Runs scripts/github-traceability-report.py
- Validates complete traceability chains
- All output captured to reports/ directory
```

### 3.2 `traceability-check.yml`

**Triggers**: On PR, issue changes, or manual dispatch

**Actions**:
- Checks for orphaned requirements
- Generates traceability report
- Comments on PRs with summary
- Uploads full report as artifact

### 3.3 `issue-validation.yml`

**Triggers**: On issue open, edit, reopen

**Actions**:
- Validates "Traces to: #N" links exist
- Checks parent issue exists
- Validates TEST issues link to requirements
- Comments on issues with errors/warnings

---

## Step 4: Verify CI Pipeline

### 4.1 Manual Workflow Trigger

Trigger workflows manually to test:

```bash
# Trigger standards compliance workflow
gh workflow run ci-standards-compliance.yml

# Trigger traceability check
gh workflow run traceability-check.yml

# Check workflow status
gh run list --workflow=ci-standards-compliance.yml
```

### 4.2 Expected Outputs (Initial State)

**Before Issues Created**:
```
📊 Requirements Status:
   - Total Requirements: 0
   - Test Cases: 0
   - Traceability: ⚠️ Needs setup

⚠️ No GitHub Issues found. Please create issues using:
   .github/ISSUE_TEMPLATE/ templates
   See: docs/QUICK-START-github-issues.md
```

**After Issues Created**:
```
📊 Requirements Status:
   - Total Requirements: 14
   - Test Cases: 13
   - Traceability: ✅ Complete

✅ #1: REQ-F-AUDIO-001: Adaptive Threshold
✅ #2: REQ-F-AUDIO-002: State Machine
...
📈 Validation Coverage: 92.9% (13/14)
```

---

## Step 5: GitHub Project Automation (Optional)

Set up project automation for auto-population of fields.

### 5.1 Enable Built-in Workflows

In project → **"⋮" → "Workflows"**:

1. **Auto-add to project**: ✅ Enable
   - Add newly opened issues automatically
2. **Item closed**: Set Status to "Done"
3. **Item reopened**: Set Status to "In Progress"

### 5.2 Custom Automation (GitHub Actions)

Create `.github/workflows/project-automation.yml`:

```yaml
name: Project Automation

on:
  issues:
    types: [opened, labeled]

jobs:
  update-project-fields:
    runs-on: ubuntu-latest
    permissions:
      issues: write
      repository-projects: write
    steps:
      - name: Add to project and set fields
        uses: actions/add-to-project@v0.5.0
        with:
          project-url: https://github.com/users/zarfld/projects/10
          github-token: ${{ secrets.PAT_PROJECT_AUTOMATION }}
          labeled: functional-requirement,non-functional,test-case
          
      - name: Set Requirement Type field
        # Use GraphQL API to set custom field based on label
        # functional-requirement → "REQ-F"
        # test-case → "TEST"
```

**Note**: Requires Personal Access Token (PAT) with `project` scope.

---

## Step 6: Traceability Matrix Generation

Once issues exist, generate traceability matrix:

```bash
# Generate matrix
python scripts/github-traceability-report.py > docs/traceability-matrix.md

# Upload to project
gh issue create --title "Traceability Matrix - $(date +%Y-%m-%d)" \
  --label "documentation" \
  --body-file docs/traceability-matrix.md
```

---

## Troubleshooting

### Issue: Workflows fail with "Rate limit exceeded"

**Solution**: GitHub API has rate limits (5000/hour authenticated)
```yaml
# Add retry logic in workflow
- uses: nick-fields/retry@v2
  with:
    timeout_minutes: 5
    max_attempts: 3
    command: python scripts/github-traceability-report.py
```

### Issue: PyGithub import error in workflow

**Solution**: Add to workflow's pip install:
```yaml
- name: Install dependencies
  run: pip install PyGithub requests pyyaml markdown
```

### Issue: Project fields not updating

**Solution**: Project API v2 requires PAT token, not GITHUB_TOKEN
- Create PAT: Settings → Developer settings → Personal access tokens
- Add to repo secrets: Settings → Secrets → Actions → `PAT_PROJECT`
- Use in workflow: `${{ secrets.PAT_PROJECT }}`

---

## Next Steps

1. ✅ **CI workflows fixed** - Dynamic GitHub Issues queries
2. 🔲 **Create GitHub Project** - Manual setup (Step 1)
3. 🔲 **Create initial issues** - 13 TEST issues + 14 REQ-F issues (Step 2)
4. 🔲 **Verify workflows** - Trigger manually and check outputs (Step 4)
5. 🔲 **Configure automation** - Project field auto-population (Step 5)

---

## References

- **Quick Start Guide**: `docs/QUICK-START-github-issues.md`
- **Issue Templates**: `.github/ISSUE_TEMPLATE/*.yml`
- **Traceability Scripts**: `scripts/github-*.py`
- **Project Manual Steps**: `docs/improvement_ideas/TASK-02-PROJECT-MANUAL-STEPS.md`
- **GitHub Projects API**: <https://docs.github.com/en/issues/planning-and-tracking-with-projects>
- **GitHub CLI**: <https://cli.github.com/manual/gh_issue_create>

---

## Summary

The CI infrastructure is now **properly automated** and will:
- ✅ Query GitHub Issues dynamically
- ✅ Validate traceability using actual scripts
- ✅ Report real status, not hardcoded values
- ✅ Generate artifacts (reports/) for auditing
- ✅ Comment on PRs with validation results

**No more hardcoded data in workflows!** 🎉

All validation is based on:
- GitHub Issues state (via GitHub API)
- Filesystem structure (actual test files)
- Script execution results (traceability reports)

This is proper **infrastructure** - automated validation of project state.
