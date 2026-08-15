---
title: Upstream PR readiness verification
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: verification
---

# Verification

## Canonical-Context Review

Reviewed 2026-08-14 against current policy authority and the expanded
task-to-context matrix. These checks still separate package planning, branch
validation, and human approval; no branch is newly claimed PR-ready.
The review covers Requirements 1 through 6 and tasks T001 through T017.

## Verification Intent

This package verifies readiness structure and evidence routing, not FreeCAD
runtime behavior. Branch submissions still need branch-specific build, test,
manual, and sign-off evidence inside their own readiness records.

## Staged Verification Status

| Gate | Status | Requirement linkage | Current note |
|------|--------|---------------------|--------------|
| Package structure | Complete | `R1`-`R6` | Core artifacts, readiness template, and per-branch records were rewritten for the current baseline; package lint now reports only a non-blocking canonical-context advisory, and package patch integrity is clean. |
| Branch validation planning | In progress | `R2`, `R4`, `R5`, `R6` | Every canonical candidate now has a record with a validation-plan placeholder, but no branch has real validation evidence yet. |
| Final branch readiness | Not ready | `R4`, `R5`, `R6` | No candidate has completed overlap research, cleanup execution, or validation. |
| Independent review readiness | Not ready | `R5`, `R6` | Human sign-off and reviewer-routing are still pending in every branch record. |

## Quality Gates

### Package-Level Quality Gates

| Gate | Scope | Required evidence | Current status |
|------|-------|-------------------|----------------|
| Baseline gate | This package revision | Recorded fetched `upstream/main` baseline and policy baseline | Complete |
| Candidate-matrix gate | This package revision | Canonical refs, alias routing, and per-branch dispositions | Complete |
| Readiness-record gate | This package revision | Template plus one record per canonical candidate | Complete |
| Package lint gate | This package revision | Spec-package lint output | Complete |
| Patch-integrity gate | This package revision | `git diff --check` limited to this package | Complete |

## Branch-Level Quality Gates

| Gate | Scope | Required evidence | Status |
|------|-------|-------------------|--------|
| Overlap research gate | Each active candidate | issue and PR overlap recorded in the branch record | Pending |
| Cleanup-plan gate | Each active candidate | explicit rebase, cherry-pick, or rebuild plan | Pending |
| Validation-plan gate | Each active candidate | `git diff --check`, build/test/manual plan matched to touched surface | In progress |
| Final validation gate | Each final PR branch | actual validation output and any justified exceptions | Pending |
| Human sign-off gate | Each final PR branch | explicit contributor sign-off and reviewer-routing notes | Pending |

## Required Command Matrix

| Check type | Default command or action | When required | Notes |
|------------|---------------------------|---------------|-------|
| Package lint | `python3 /home/bcherrington/.codex/plugins/cache/auriora-local/spec-lifecycle-manager/0.6.0/skills/spec-lifecycle-manager/scripts/spec_runtime.py lint docs/specs/002-upstream-pr-readiness` | After package-structure edits | Required for this revision. |
| Package patch integrity | `git diff --check -- docs/specs/002-upstream-pr-readiness` | After package edits | Required for this revision. |
| Branch patch integrity | `git diff --check` | For each cleaned final PR branch | Record inside the branch record. |
| Build | `pixi run build` | Default for final PR branches | Narrower commands must be justified in the record. |
| Focused tests | branch-specific `ctest`, target build, or Python test command | When the branch touches a testable component | Record exact command in the branch record. |
| Manual GUI/startup evidence | screenshot, recording, or bounded checklist | GUI-visible or startup branches | Required for toolbar, overlay, image-plane, tree-status, bitmap, and splash candidates. |
| Performance evidence | before/after workload and variance notes | Performance candidates | Required before performance branches become PR-ready. |

## Evidence Log

| Date | Tasks | Requirements | Evidence | Result | Notes |
|------|-------|--------------|----------|--------|-------|
| 2026-08-07 | `T001` | `R1`, `R3` | `git fetch upstream main` plus baseline inspection | Complete | Current review-cycle baseline recorded as `7937058dededc800dd0f3197185daf75d146c9d6`. |
| 2026-08-07 | `T002` | `R3` | Local policy reads of `CONTRIBUTING.md`, `.github/pull_request_template.md`, and `AI_POLICY.md` | Complete | Current binding policy baseline recorded in [research.md](research.md). |
| 2026-08-07 | `T003` | `R3` | Official PR and handbook page review | Complete | Current policy-discussion watch items and handbook references cited in [research.md](research.md). |
| 2026-08-07 | `T004`, `T005` | `R1`, `R4`, `R6` | Candidate branch inventory and alias routing inspection | Complete | Canonical candidate matrix and alias routing recorded in [research.md](research.md). |
| 2026-08-07 | `T006` | `R2`, `R4`, `R5`, `R6` | Readiness template and per-branch record creation | Complete | Records added under `readiness-records/`. |
| 2026-08-07 | package verification | `R1`, `R2`, `R3`, `R4`, `R5`, `R6` | `python3 /home/bcherrington/.codex/plugins/cache/auriora-local/spec-lifecycle-manager/0.6.0/skills/spec-lifecycle-manager/scripts/spec_runtime.py lint docs/specs/002-upstream-pr-readiness` | Complete with advisory | The lint run reported only `CANONICAL_CONTEXT_MISSING`, marked non-blocking/advisory. |
| 2026-08-07 | package verification | `R1`, `R2`, `R3`, `R4`, `R5`, `R6` | `git diff --check -- docs/specs/002-upstream-pr-readiness` | Complete | Command exited cleanly with no reported patch-format issues. |

## Residual Risks

- The candidate matrix will drift again if `upstream/main` moves before the next
  branch-specific slice.
- The package currently proves planning readiness, not branch readiness.
- Performance branches still need careful split/rebuild work to avoid leaking
  unrelated local docs/spec history.
- Open policy-discussion PRs may still change the preferred PR framing later.
- `canonical-context.md` was not added in this slice; the current lint result
  treats that as an advisory rather than a package blocker.

## Closure Criteria

- Every canonical candidate branch is routed to ready, deferred, blocked, or
  obsolete with evidence.
- At least one clean PR-preparation workflow is exercised and recorded well
  enough to support durable promotion or closure.
- Package lint and patch-integrity evidence are recorded.
- No active branch remains without a readiness record.
