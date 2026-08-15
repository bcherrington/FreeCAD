---
title: Upstream PR readiness traceability
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: traceability
---

# Traceability

## Canonical-Context Review

Reviewed 2026-08-14 after design and task authority reconciliation. Every task
now has an individual context row; pending work and branch readiness claims are
unchanged.

## Package Identity

- Current package: `docs/specs/002-upstream-pr-readiness/`
- Legacy package ID: `upstream-pr-readiness`

## Requirement To Delivery Matrix

| Requirement | Acceptance criteria | Design coverage | Task coverage | Verification coverage |
|-------------|---------------------|-----------------|---------------|-----------------------|
| `R1` | `AC1.1`, `AC1.2`, `AC1.3` | overview, artifact layout, canonical disposition model | `T001`, `T004`, `T005` | package structure gate, evidence log |
| `R2` | `AC2.1`, `AC2.2`, `AC2.3` | artifact layout, readiness record schema, dependency model | `T006`, `T007`-`T014` | branch validation planning gate |
| `R3` | `AC3.1`, `AC3.2`, `AC3.3` | durable baseline and impact, workflow architecture | `T001`, `T002`, `T003` | policy baseline gate |
| `R4` | `AC4.1`, `AC4.2`, `AC4.3` | canonical disposition model, dependency model | `T004`, `T005`, `T007`-`T016` | final branch gate |
| `R5` | `AC5.1`, `AC5.2`, `AC5.3` | validation strategy, independent review strategy | `T006`, `T007`-`T016` | validation planning gate, command matrix |
| `R6` | `AC6.1`, `AC6.2`, `AC6.3` | staged readiness, independent review strategy, operational considerations | `T005`, `T006`, `T007`-`T017` | independent review gate, human sign-off gate |

## Design To Implementation Matrix

| Design element | Implemented by |
|----------------|----------------|
| Review-cycle baseline | [requirements.md](requirements.md), [research.md](research.md), [tasks.md](tasks.md) |
| Shared policy baseline | [research.md](research.md), [tasks.md](tasks.md) |
| Canonical candidate matrix | [research.md](research.md) |
| Canonical disposition model | [design.md](design.md), [research.md](research.md) |
| Reusable readiness schema | [readiness-record-template.md](readiness-record-template.md) |
| Per-branch readiness records | `readiness-records/*.md` |
| Validation gate separation | [verification.md](verification.md), `readiness-records/*.md` |

## Task To Context Matrix

| Task ID | Requirements | Acceptance Criteria | Design Sections | Verification | Durable Targets | Open Decisions |
| --- | --- | --- | --- | --- | --- | --- |
| T001 | Requirement 1; Requirement 3 | AC1.1-AC1.3; AC3.1-AC3.3 | Review-cycle baseline | package baseline gate | `research.md` | none |
| T002 | Requirement 3 | AC3.1-AC3.3 | Shared policy baseline | policy baseline gate | `research.md`; `canonical-context.md` | current contribution-policy authority |
| T003 | Requirement 3 | AC3.1-AC3.3 | Shared policy baseline | policy baseline gate | `research.md` | AI-policy watch-item status |
| T004 | Requirement 1; Requirement 4 | AC1.1-AC1.3; AC4.1-AC4.3 | Canonical disposition model | package structure gate | `research.md` | canonical candidate set |
| T005 | Requirement 1; Requirement 4; Requirement 6 | AC1.1-AC1.3; AC4.1-AC4.3; AC6.1-AC6.3 | Canonical disposition model; dependency model | package structure gate | `research.md` | alias and obsolete routing |
| T006 | Requirement 2; Requirement 4; Requirement 5; Requirement 6 | AC2.1-AC2.3; AC4.1-AC6.3 | Readiness record schema; validation strategy | validation planning gate | readiness record template and records | active candidate set |
| T007 | Requirement 2; Requirement 4; Requirement 5; Requirement 6 | AC2.1-AC2.3; AC4.1-AC6.3 | Readiness record schema; dependency model; validation strategy | branch validation planning gate | toolbar readiness record | overlap and cleanup disposition |
| T008 | Requirement 2; Requirement 4; Requirement 5; Requirement 6 | AC2.1-AC2.3; AC4.1-AC6.3 | Readiness record schema; dependency model; validation strategy | branch validation planning gate | overlay readiness record | overlap and cleanup disposition |
| T009 | Requirement 2; Requirement 4; Requirement 5; Requirement 6 | AC2.1-AC2.3; AC4.1-AC6.3 | Readiness record schema; dependency model; validation strategy | branch validation planning gate | included-file restore readiness record | overlap and cleanup disposition |
| T010 | Requirement 2; Requirement 4; Requirement 5; Requirement 6 | AC2.1-AC2.3; AC4.1-AC6.3 | Readiness record schema; dependency model; validation strategy | branch validation planning gate | element-map readiness record | overlap and cleanup disposition |
| T011 | Requirement 2; Requirement 4; Requirement 5; Requirement 6 | AC2.1-AC2.3; AC4.1-AC6.3 | Readiness record schema; dependency model; validation strategy | branch validation planning gate | tree-status readiness record | overlap and cleanup disposition |
| T012 | Requirement 2; Requirement 4; Requirement 5; Requirement 6 | AC2.1-AC2.3; AC4.1-AC6.3 | Readiness record schema; dependency model; validation strategy | branch validation planning gate | image-plane readiness record | overlap and cleanup disposition |
| T013 | Requirement 2; Requirement 4; Requirement 5; Requirement 6 | AC2.1-AC2.3; AC4.1-AC6.3 | Readiness record schema; dependency model; validation strategy | branch validation planning gate | bitmap readiness record | upstream-overlap disposition |
| T014 | Requirement 2; Requirement 4; Requirement 5; Requirement 6 | AC2.1-AC2.3; AC4.1-AC6.3 | Readiness record schema; dependency model; validation strategy | branch validation planning gate | splashscreen readiness record | platform validation scope |
| T015 | Requirement 5; Requirement 6 | AC5.1-AC6.3 | Validation strategy; staged readiness | command matrix and branch gate | selected readiness record | first planning-complete candidate |
| T016 | Requirement 4; Requirement 5 | AC4.1-AC5.3 | Dependency model; final branch gate | clean branch gate | selected readiness record | cleanup path |
| T017 | Requirement 6 | AC6.1-AC6.3 | Independent review strategy | human sign-off gate | selected readiness record | explicit human approval |

## Open Decision Impact

| Open decision | Affected requirements | Affected tasks | Current impact |
|---------------|-----------------------|----------------|----------------|
| Whether all eight active candidates still deserve upstream PR effort | `R1`, `R2`, `R4`, `R6` | `T007`-`T017` | May reduce the active set before any clean PR branch is prepared. |
| Whether `perf/bitmap-conversion-fast-path` still offers enough value beyond current upstream behavior | `R2`, `R4`, `R5` | `T013` | Could reroute the branch to deferred or obsolete. |
| Which candidates require non-Linux validation before they are defensible | `R5`, `R6` | `T007`-`T017` | May block later PR preparation even after local validation planning is complete. |

## Correctness Property Coverage

| Property | Protected by | Notes |
|----------|--------------|-------|
| `CP-001` | package identity, staged readiness, task dependency graph | This package revision does not authorize PR creation or source edits. |
| `CP-002` | verification evidence log, per-branch records, human sign-off fields | Package updates alone cannot prove branch readiness. |
| `CP-003` | this file, task-to-record mapping, candidate matrix | The package remains navigable after replacing the old umbrella task list. |
