---
title: Upstream PR readiness tasks
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: tasks
---

# Tasks

## Canonical-Context Review

Reviewed 2026-08-14 after canonical authority and task-level traceability were
made explicit. Pending task state and dependency ordering remain unchanged.

## Package Identity

- Current package: `docs/specs/002-upstream-pr-readiness/`
- Legacy package ID: `upstream-pr-readiness`
- This task list replaces the stale 72-task umbrella with one baseline phase,
  one inventory phase, and one readiness task per active branch.

## Dependency Graph

```text
T001-T003 -> T004-T006 -> T007-T014 -> T015-T017

T001-T003: review-cycle baseline and policy
T004-T006: candidate matrix, alias routing, readiness schema
T007-T014: branch-specific readiness completion
T015-T017: validation, clean PR branch prep, and human sign-off
```

## Phase 1: Review-Cycle Baseline

- [x] T001 Record the fetched `upstream/main` baseline for this review cycle.
  - Depends on: None.
  - Requirement: `R1`, `R3`
  - Files: [research.md](research.md), [requirements.md](requirements.md)
  - Acceptance: The package records the fetched baseline commit, date, and
    subject used for current branch reconciliation.
  - Evidence: `git fetch upstream main`; `git rev-parse upstream/main`; `git log -1 --format='%H %cs %s' upstream/main`
- [x] T002 Summarize the currently binding contribution-policy baseline.
  - Depends on: T001.
  - Requirement: `R3`
  - Files: [research.md](research.md)
  - Acceptance: The package summarizes `CONTRIBUTING.md`,
    `.github/pull_request_template.md`, `AI_POLICY.md`, and the official
    Developers Handbook pages used for PR preparation.
  - Evidence: direct reads of the local files plus the cited official handbook
    pages checked on 2026-08-07.
- [x] T003 Reconcile the current status of AI-policy watch-item PRs.
  - Depends on: T001.
  - Requirement: `R3`
  - Files: [research.md](research.md)
  - Acceptance: The package records the current GitHub status of PR `#30268`
    and PR `#30273`, and treats them as watch items instead of replacing the
    current merged local policy baseline.
  - Evidence: GitHub PR pages checked on 2026-08-07 and cited in the package.

## Phase 2: Candidate Inventory And Schema

- [x] T004 Reconcile canonical candidate refs, legacy names, and branch-shape facts.
  - Depends on: T001.
  - Requirement: `R1`, `R4`
  - Files: [research.md](research.md)
  - Acceptance: The package contains a current matrix with canonical refs,
    ahead/behind counts, branch-shape notes, and dispositions.
  - Evidence: `git branch --list`; `git rev-list --left-right --count`; `git log`; `git diff --stat`
- [x] T005 Route alias-only, composite, or obsolete local refs out of the active path.
  - Depends on: T004.
  - Requirement: `R1`, `R4`, `R6`
  - Files: [research.md](research.md)
  - Acceptance: The package explicitly routes `perf-included-file-restore-buffering`
    and other non-canonical local refs instead of treating them as active PR
    sources.
  - Evidence: `git cherry -v upstream/main <branch>` and current local branch inventory.
- [x] T006 Create a reusable readiness schema and per-branch records for the active set.
  - Depends on: T002, T004, T005.
  - Requirement: `R2`, `R4`, `R5`, `R6`
  - Files: [readiness-record-template.md](readiness-record-template.md),
    `readiness-records/*.md`
  - Acceptance: A reusable template exists and each canonical active candidate
    has a record with inventory, cleanup, validation-plan, and next-action
    placeholders.
  - Evidence: package-local record files added in this revision.

## Phase 3: Active Candidate Readiness Work

- [ ] T007 Complete issue, overlap, cleanup, and validation planning for `toolbar-persistence-save-settings`.
  - Depends on: T006.
  - Requirement: `R2`, `R4`, `R5`, `R6`
  - Files: [readiness-records/toolbar-persistence-save-settings.md](readiness-records/toolbar-persistence-save-settings.md)
  - Acceptance: The record is updated from inventory-only status to a full
    branch-specific readiness plan with issue/PR overlap and validation details.
  - Evidence: Pending.
- [ ] T008 Complete issue, overlap, cleanup, and validation planning for `overlay-tab-destruction-cleanup`.
  - Depends on: T006.
  - Requirement: `R2`, `R4`, `R5`, `R6`
  - Files: [readiness-records/overlay-tab-destruction-cleanup.md](readiness-records/overlay-tab-destruction-cleanup.md)
  - Acceptance: The record captures merge-cleanup strategy and branch-specific
    review concerns.
  - Evidence: Pending.
- [ ] T009 Complete issue, overlap, cleanup, and validation planning for `perf/included-file-restore-buffering`.
  - Depends on: T006.
  - Requirement: `R2`, `R4`, `R5`, `R6`
  - Files: [readiness-records/perf-included-file-restore-buffering.md](readiness-records/perf-included-file-restore-buffering.md)
  - Acceptance: The record confirms the slash-named restore branch as the
    active source and records the cleanup/rebuild strategy.
  - Evidence: Pending.
- [ ] T010 Complete issue, overlap, cleanup, and validation planning for `perf/element-map-restore-parsing`.
  - Depends on: T006.
  - Requirement: `R2`, `R4`, `R5`, `R6`
  - Files: [readiness-records/perf-element-map-restore-parsing.md](readiness-records/perf-element-map-restore-parsing.md)
  - Acceptance: The record states how the functional commit will be separated
    from local docs/spec history and what validation is required.
  - Evidence: Pending.
- [ ] T011 Complete issue, overlap, cleanup, and validation planning for `perf/tree-status-startup-followup`.
  - Depends on: T006.
  - Requirement: `R2`, `R4`, `R5`, `R6`
  - Files: [readiness-records/perf-tree-status-startup-followup.md](readiness-records/perf-tree-status-startup-followup.md)
  - Acceptance: The record reflects the slash-named branch and records GUI risk
    and docs/spec cleanup strategy.
  - Evidence: Pending.
- [ ] T012 Complete issue, overlap, cleanup, and validation planning for `perf/image-plane-lifecycle-lazy-texture`.
  - Depends on: T006.
  - Requirement: `R2`, `R4`, `R5`, `R6`
  - Files: [readiness-records/perf-image-plane-lifecycle-lazy-texture.md](readiness-records/perf-image-plane-lifecycle-lazy-texture.md)
  - Acceptance: The record separates the two functional commits from unrelated
    local docs/spec commits and records the GUI validation path.
  - Evidence: Pending.
- [ ] T013 Complete issue, overlap, cleanup, and validation planning for `perf/bitmap-conversion-fast-path`.
  - Depends on: T006.
  - Requirement: `R2`, `R4`, `R5`, `R6`
  - Files: [readiness-records/perf-bitmap-conversion-fast-path.md](readiness-records/perf-bitmap-conversion-fast-path.md)
  - Acceptance: The record captures upstream-overlap risk, test scope, and the
    cleanup path for unrelated local docs/spec commits.
  - Evidence: Pending.
- [ ] T014 Complete issue, overlap, cleanup, and validation planning for `16264-splashscreen-refactor`.
  - Depends on: T006.
  - Requirement: `R2`, `R4`, `R5`, `R6`
  - Files: [readiness-records/16264-splashscreen-refactor.md](readiness-records/16264-splashscreen-refactor.md)
  - Acceptance: The record captures startup-platform risk, likely reviewer
    routing, and why this should remain a later PR candidate unless evidence
    improves.
  - Evidence: Pending.

## Phase 4: Validation And Submission Gates

- [ ] T015 Run branch-specific validation for the first candidate that reaches planning-complete state.
  - Depends on: one of T007-T014.
  - Requirement: `R5`, `R6`
  - Files: the selected readiness record and any cited artifacts
  - Acceptance: The selected branch record contains actual validation evidence,
    not just planned commands.
  - Evidence: Pending.
- [ ] T016 Prepare a clean final PR branch for the first candidate that passes validation planning.
  - Depends on: T015.
  - Requirement: `R4`, `R5`
  - Files: the selected readiness record
  - Acceptance: The record states the exact cleanup path used to create the
    submission branch and confirms unrelated local history was excluded.
  - Evidence: Pending.
- [ ] T017 Obtain explicit human sign-off before any push or PR creation.
  - Depends on: T016.
  - Requirement: `R6`
  - Files: the selected readiness record
  - Acceptance: The record shows human review/sign-off distinct from command
    evidence and reviewer-routing notes.
  - Evidence: Pending.
