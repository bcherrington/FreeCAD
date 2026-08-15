---
title: Upstream PR readiness for FreeCAD topic branches requirements
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: requirements
---

# Upstream PR readiness for FreeCAD topic branches

## Package Identity

- Current package: `docs/specs/002-upstream-pr-readiness/`
- Legacy package ID: `upstream-pr-readiness`
- Package intent: prepare selected local FreeCAD topic branches for upstream PR
  submission without widening into feature implementation, branch mutation, or
  PR creation in this slice.

## Current Review-Cycle Baseline

- Fetched baseline ref: `upstream/main`
- Fetched baseline commit: `7937058dededc800dd0f3197185daf75d146c9d6`
- Baseline commit date: `2026-08-07`
- Baseline commit subject: `Make it possible to invoke multiple tests at once (#31402)`
- Current package truth is the combination of this file,
  [design.md](design.md), [research.md](research.md), [tasks.md](tasks.md),
  [verification.md](verification.md), [traceability.md](traceability.md),
  [readiness-record-template.md](readiness-record-template.md), and the
  branch records under
  [readiness-records/](readiness-records/toolbar-persistence-save-settings.md).

## Durable Source Baseline

- Local repository policy files:
  - `CONTRIBUTING.md`
  - `.github/pull_request_template.md`
  - `AI_POLICY.md`
- Official FreeCAD contributor guidance consulted on 2026-08-07:
  - [FreeCAD Developers Handbook: Getting Started / Submitting a PR](https://freecad.github.io/DevelopersHandbook/gettingstarted/)
  - [FreeCAD Developers Handbook: Maintainers Guide](https://freecad.github.io/DevelopersHandbook/maintainersguide/)
- Official FreeCAD policy-discussion PRs checked on 2026-08-07:
  - [PR #30268](https://github.com/FreeCAD/FreeCAD/pull/30268)
  - [PR #30273](https://github.com/FreeCAD/FreeCAD/pull/30273)

## Durable Impact

- This package does not itself change FreeCAD runtime behavior.
- It records which local branches are still plausible upstream PR candidates,
  which local names are obsolete or alias-only, and what evidence each real
  candidate still needs.
- Before closure, any reusable contributor workflow discovered here must be
  promoted to durable project documentation or a closure record.

## Staged Readiness

| Gate | Current status | Why |
|------|----------------|-----|
| `ready_to_implement` | Ready | The package now records the fetched `upstream/main` baseline, current policy baseline, canonical candidate inventory, alias/obsolete routing, a readiness template, and per-branch records for the live candidate set. |
| `ready_to_validate` | Not ready | No candidate branch has completed issue/PR overlap review, branch-specific validation, and human sign-off yet. |
| `ready_to_close` | Not ready | Candidate branches remain in mixed states: some are clean, some need branch cleanup, and some remain blocked on validation or issue research. |
| `ready_to_archive` | Not ready | The package remains the active coordination surface for this work. |

## Context

The old 72-task umbrella became stale because the candidate list drifted from
current branch names, several performance branches accumulated unrelated local
docs/spec commits, one included-file branch split into a cleaner slash-named
variant, and the package did not contain branch-specific records that could be
updated independently. This revision makes the package executable again by
separating:

- current policy baseline from historical policy-discussion watch items;
- canonical candidate refs from stale or alias names;
- package-wide workflow rules from branch-specific readiness records; and
- active candidates from deferred, blocked, or obsolete routing.

## Goals

- Keep a current candidate matrix tied to one fetched `upstream/main` baseline.
- Require a branch-specific readiness record before a branch is treated as
  submission-ready.
- Make branch cleanup decisions explicit where local docs/spec or merge commits
  contaminate a candidate.
- Preserve issue research, PR overlap, validation planning, reviewer routing,
  and human sign-off as separate gates.
- Route obsolete or alias-only branch names out of the active path.

## Non-Goals

- Open, push, or submit upstream PRs from this package alone.
- Treat a local integration branch or package update as evidence that a code
  branch is ready.
- Rework source code, tests, or history inside this slice.
- Reconstruct historical intent for every branch when the current Git state
  already shows a safer route.

## Glossary

- **Canonical candidate branch**: The current local branch ref that should be
  used if that change still merits upstream preparation.
- **Alias branch**: A local branch name retained for history or convenience but
  not the preferred PR source.
- **Readiness record**: A package-local file for one canonical candidate branch.
- **Composite branch**: A local branch that bundles more than one logical change
  or carries unrelated docs/spec history and therefore needs cleanup or split
  routing before PR preparation.

## Success Criteria

- The package names one fetched `upstream/main` baseline and uses it consistently.
- The package distinguishes canonical candidate refs from alias-only or obsolete
  local names.
- Every canonical candidate branch has a readiness record.
- The next branch-specific slice can start from a single record without
  re-deriving the whole inventory.
- No package artifact implies that a branch is PR-ready without overlap review,
  validation, and human sign-off.

## Requirements

### Requirement 1: Canonical Candidate Inventory Must Be Current

**ID:** `R1`

**Priority:** must-have

**User Story:** As the contributor, I want the package to reflect the current
local branch inventory and naming, so that I do not prepare PRs from stale or
misnamed refs.

#### Acceptance Criteria

- **AC1.1** GIVEN the package is active, WHEN the review cycle starts, THEN it
  SHALL record the fetched `upstream/main` baseline commit used for the cycle.
- **AC1.2** GIVEN legacy branch names differ from current refs, WHEN the
  candidate matrix is updated, THEN canonical refs and alias-only names SHALL be
  distinguished explicitly.
- **AC1.3** GIVEN a branch is not an active candidate, WHEN it is discussed in
  this package, THEN it SHALL be routed as `alias-only`, `obsolete`,
  `superseded`, or `deferred`, not left ambiguous.

### Requirement 2: Every Active Candidate Must Have A Readiness Record

**ID:** `R2`

**Priority:** must-have

**User Story:** As a FreeCAD maintainer, I want every active candidate to carry
its own readiness record, so that review context and evidence can be updated
without touching an unrelated branch.

#### Acceptance Criteria

- **AC2.1** GIVEN a canonical candidate branch, WHEN the package is reconciled,
  THEN a branch-specific readiness record SHALL exist under
  `docs/specs/002-upstream-pr-readiness/readiness-records/`.
- **AC2.2** GIVEN a readiness record exists, WHEN it is reviewed, THEN it SHALL
  include branch inventory facts, cleanup notes, validation planning, and a next
  action.
- **AC2.3** IF a candidate lacks a readiness record, THEN it SHALL NOT be
  marked ready for upstream submission in this package.

### Requirement 3: Readiness Review Must Use Current Contribution Rules

**ID:** `R3`

**Priority:** must-have

**User Story:** As a maintainer, I want readiness decisions checked against the
current contribution process, so that PR preparation matches live upstream
expectations.

#### Acceptance Criteria

- **AC3.1** GIVEN a review cycle begins, WHEN policy research is recorded, THEN
  the package SHALL summarize the relevant expectations from `CONTRIBUTING.md`,
  `.github/pull_request_template.md`, `AI_POLICY.md`, and the official
  Developers Handbook pages used for PR preparation.
- **AC3.2** GIVEN policy-discussion PRs remain open, WHEN they are recorded,
  THEN the package SHALL treat them as watch items rather than replacing the
  current merged local policy files.
- **AC3.3** GIVEN a PR draft is eventually prepared, WHEN it is reviewed, THEN
  the readiness record SHALL note whether open policy-discussion PRs changed the
  submission approach.

### Requirement 4: Cleanup And Dependency Routing Must Be Explicit

**ID:** `R4`

**Priority:** must-have

**User Story:** As a reviewer, I want each candidate branch to have a clear
cleanup path, so that local merge commits, docs/spec commits, and dependent
branch chains do not leak into an upstream PR.

#### Acceptance Criteria

- **AC4.1** GIVEN a candidate branch contains merge commits or unrelated
  docs/spec commits, WHEN its readiness record is updated, THEN the record SHALL
  say whether to rebase, cherry-pick functional commits, or rebuild a clean
  branch.
- **AC4.2** GIVEN a composite branch has a cleaner canonical replacement, WHEN
  the matrix is updated, THEN the composite branch SHALL be routed away from the
  active path.
- **AC4.3** GIVEN a branch depends on earlier local work, WHEN readiness is
  recorded, THEN the dependency or sequencing requirement SHALL be captured.

### Requirement 5: Validation Planning Must Match The Changed Surface

**ID:** `R5`

**Priority:** must-have

**User Story:** As a maintainer, I want validation expectations mapped to the
actual touched files, so that each PR candidate has proportional evidence.

#### Acceptance Criteria

- **AC5.1** GIVEN a candidate branch is kept active, WHEN its readiness record
  is reviewed, THEN the record SHALL include `git diff --check` and either
  `pixi run build` or a justified narrower build/test plan.
- **AC5.2** GIVEN a branch changes GUI-visible or startup behavior, WHEN its
  plan is recorded, THEN manual GUI evidence and reviewer-routing expectations
  SHALL be named.
- **AC5.3** GIVEN a performance branch is kept active, WHEN its plan is
  recorded, THEN the record SHALL call for before/after workload evidence and
  variance notes before submission-ready status.

### Requirement 6: Human Review And Sign-Off Must Remain Separate Gates

**ID:** `R6`

**Priority:** must-have

**User Story:** As the human contributor, I want automated checks, reviewer
  routing, and my own sign-off kept separate, so that no branch is treated as
  ready merely because a package task was updated.

#### Acceptance Criteria

- **AC6.1** GIVEN a candidate branch is close to PR-ready, WHEN the readiness
  record is reviewed, THEN it SHALL still show independent-review expectations
  and human sign-off status separately from command evidence.
- **AC6.2** GIVEN a branch lacks issue research, PR overlap review, validation,
  or human sign-off, WHEN the package is reviewed, THEN that branch SHALL remain
  `not ready`.
- **AC6.3** IF a branch is deferred, blocked, or obsolete, THEN the package
  SHALL route it explicitly instead of implying it is waiting only on a final
  push.

## Correctness Properties

| Property | Description |
|----------|-------------|
| `CP-001` | This package revision must stay inside `docs/specs/002-upstream-pr-readiness/**` and must not imply source, test, or Git history changes happened. |
| `CP-002` | Canonical candidate refs must be distinguishable from alias-only, obsolete, or composite local names. |
| `CP-003` | Package-level evidence must not be confused with branch-level validation evidence or human sign-off. |
