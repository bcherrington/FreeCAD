---
title: Sketcher external reference reattachment canonical context
doc_type: spec
artifact_type: canonical-context
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# Canonical Context

## Purpose

Separate the proposed API contract, verified source observations and external
handoff reports. Prevent planning and structural checks from becoming claims of
implemented native behavior or current live-runtime authority.

## Authority Hierarchy

Root AGENTS.md, applicable deeper instructions and current source govern execution.
Repository `docs/templates/spec-package/` governs package format. Read
[requirements](requirements.md), [design](design.md), [tasks](tasks.md),
[traceability](traceability.md) and [verification](verification.md) together.
Requirements are canonical acceptance; design is proposed implementation direction;
tasks are ordered work; verification distinguishes plans from executed evidence.

This is a new package, not a migration or reopening of archived spec 000. The
brief is navigation only. No legacy plan, imported transcript or runtime receipt
is made authoritative by copying it into the package.

## Always-Canonical External Sources

| Source | Authority reason | Handling |
| --- | --- | --- |
| AGENTS.md and deeper instructions | Execution rules | Recheck before changing source |
| CONTRIBUTING.md and AI_POLICY.md | Contribution obligations | Human review and current policy at public preparation |
| docs/templates/spec-package/README.md | Package authoring contract | Preserve stage, status and evidence semantics |
| Current Sketcher source and tests | Implemented behavior | Reconcile against the source baseline below |

## Spec-Canonical Working Sources

| Source | Role | Scope |
| --- | --- | --- |
| requirements.md | Proposed acceptance | Requirements 1–5 and CP-001–CP-005 |
| design.md | Proposed implementation | Bounded native/Python operation |
| open-decisions.md | Authoritative decision status and ownership | D001 and D002 |
| tasks.md | Dependency-ordered work | T001–T006 |
| traceability.md | Bidirectional coverage | All requirements/properties |
| verification.md | Validation plan and executed evidence | Named checks only |

## User Intent and Provenance

The user requested a spec after reviewing whether native Sketcher external
reference reattachment could be exposed to Python consistently with FreeCAD
implementation patterns. Motivation is the November 7 modelling-agent handoff in
`artifacts/freecad-crash-coordination/COORDINATION.txt` in the separate
`freecad-modelling-mcp` repository. The handoff requests preservation of native
external geometry IDs, order, flags and dependent constraint endpoint roles,
including recompute/save/reopen/undo qualification.

The external file is background evidence, not a build identity, live document
authority or permission to edit another repository. Do not copy historical PIDs,
runtime identifiers or operator CAD into automated tests. The public regression
must be generated from minimal analytic geometry within this repository.

## Imported Sources

| Spec path | Source | Source date | Handling | Canonical scope |
| --- | --- | --- | --- | --- |
| requirements.md; design.md | User request and external coordination handoff | 2026-11-07 | Summarized intent only | Proposed workflow and preservation needs |
| Source baseline below | Direct local source reads | 2026-11-07 | Hash-pinned observations | Observed implementation only |

## Non-Canonical Background Sources

The external coordination log, old runtime identities and bridge delivery claims
remain historical background. They cannot prove the new native contract or the
state of any current user document. spec.md is navigation, not separate acceptance.

## Source Baseline

Observed 2026-11-07 on branch `docs/reconcile-spec-formats`.
Root HEAD at inspection: `d4ddd049df020b327f6a4154f89d39fc04639e9b`;
nested docs HEAD: `7769845a2910298f49af7d28afdcc4631a96b23a`.
Other work advanced during this session: these are observation anchors, not a
promise that HEAD is still unchanged. Reconcile before implementation.

| Source | SHA256 at inspection |
| --- | --- |
| src/Mod/Sketcher/App/SketchObjectExternal.cpp | 3fda020075e862e734482f1d2b6cd20daa9e70b1931b51a0e4a52a5ced8a9913 |
| src/Mod/Sketcher/App/SketchObjectPyImp.cpp | d8fa71b1f857a3bc2ef9091f0d6c43c2530bb2a90ce9ba6d6e37c7b01f64d35f |
| src/Mod/Sketcher/App/SketchObject.pyi | 62641cb4a54ed5273a95e4475732325c9d33153783b4aea0485b2cd9752042a7 |
| src/Mod/Sketcher/SketcherTests/TestSketcherSolver.py | 6eb4a185275b7c2713d42fe9f15c3200d2d6047403ef060bfe881c8f1c4a1923 |

Source observations:

- Native attachExternal accepts a vector of geometry indices, expands related
  projections, erases old links and appends a new link before rebuilding.
- Rebuilding associates projected geometry with native IDs and retains existing
  flags, but may delete projections when cardinality decreases.
- Existing Python addExternal/carbonCopy wrappers use native eligibility checks
  and error translation. No attachExternal wrapper was found in inspected source.
- SketchObject.pyi and `generate_from_py(SketchObject)` in the App CMake file are
  the actual binding mechanism; a SketchObjectPy.xml file was not found here.
- Existing tests exercise source projections, caller transactions and undo/redo.
- Native tests live under `tests/src/Mod/Sketcher/App/`; their parent target is
  Sketcher_tests_run. Confirm build output and test names during T001.

These observations justify a proposed design, not a proven preservation guarantee.

## Worktree and Ownership

At intake the worktree contained unrelated Python-console documentation, spec 003
and PropertyFile test work. Concurrent changes were later committed by their
owner during inspection. This task owns only this new package and its index row.
Do not revert, stage, commit or relabel other work. The nested docs repository
has its own Git identity and must be included in any future source/docs evidence.

No execution dependency on specs 001–003 is established. Shared Sketcher source
changes require reconciliation with current code, not waiting for unrelated
performance or GUI delivery. Spec 004 makes no crash or speedup claim.

## Decisions and Readiness

D001 selects reattachExternal. The user explicitly accepted D002 Option A after
T001 found that existing guards do not meet strict atomicity. The scalar API takes
a negative geometry index and admits compatible one-to-one Projection-mode
line/circle/arc references. Reversed or ambiguous mappings, grouped/intersection
references and Frozen/Missing/Detached/noRecomputes states reject in preflight.

On success, native IDs, both orders, flags, ExternalTypes, authored constraints
including exact Orientation, and Ref/dependency consistency are required. Preflight
rejection and admitted no-op preserve model/journal state without notifications.
During application, ordinary intermediate property notifications are allowed.
Unexpected apply errors propagate for caller-owned transaction abort or checkpoint
recovery; partial changes and dirty/redo effects are possible. Aborting a shared
transaction also reverts earlier edits. No operation-local savepoint is promised.

T001 qualifies the native/API boundary, generated fixtures, exact existing runners
and ordinary caller abort after an injected native application exception. It does
not implement or validate the new API. See [the report](t001-qualification.md) and
[review disposition](verification.md#moe-review-disposition). Generic atomic App
infrastructure is explicitly outside the user-approved slice.

The user authorized T001 and its Option A reconciliation. Neither authorizes
installing a snapshot, restarting an operator runtime, changing a live document
or submitting upstream. Structural lint is distinct from implementation acceptance.

## Shared-Worktree Execution

Another agent is working in this checkout. Keep the currently checked-out branch;
do not switch, reset or stash it for this feature. Agree file ownership before any
overlapping source edits and preserve unrelated work. After implementation and
validation, prepare the clean upstream PR branch in a separate Git worktree,
cherry-picking only feature commits. Spec 004 remains local delivery scaffolding
unless separately selected for public documentation. This records the user's
explicit workflow correction and supersedes generic named-branch wording.

## Promotion Map

Promote accepted API details into SketchObject.pyi and
`docs/reference/sketcher-external-reference-reattachment.md`. Record evidence and
final package disposition in the existing history indexes. The durable reference
must distinguish delivered behavior from unsupported broader integration work.

| Working content | Durable destination | Promotion condition |
| --- | --- | --- |
| Signature, input/index space and errors | src/Mod/Sketcher/App/SketchObject.pyi | Implemented and tested in T003–T005 |
| Accepted guarantees, limits and lifecycle examples | docs/reference/sketcher-external-reference-reattachment.md | Qualified in T004/T005, promoted in T006 |
| Final evidence, residual disposition and commits | Existing closure log and archive index | Closure gate and actual Git evidence |

## Smallest Next Action

T002's native implementation and direct C++ evidence are recorded in
[t002-implementation.md](t002-implementation.md) and
[t002-validation.json](t002-validation.json). Obtain fresh context for T003 -
Expose and document the public Python binding. Preserve the shared branch/file
ownership boundaries. T004 also owns the recorded baseline Coincident solver
finding; do not infer whole-feature qualification from T002 native test passes.
