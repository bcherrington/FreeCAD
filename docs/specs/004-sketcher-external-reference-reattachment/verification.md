---
title: Sketcher external reference reattachment verification
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# Verification

## Scope

Requirements 1–5, CP-001–CP-005 and T001–T006. This is a proposed validation
plan, spec-authoring record and T001 qualification evidence. Eight standalone
baseline probes, six existing native cases, five existing Python cases and one
arc fixture have run. No new API, T002 implementation, operator-runtime activation
or live model retargeting has been performed.

## Quality Gates

| Gate | Required? | Status | Evidence |
| --- | --- | --- | --- |
| Requirements and proposed design present | yes | drafted; MoE corrections applied | requirements.md; design.md; review disposition below |
| D001 and D002 qualified before implementation | yes | D001 selected locally; user accepted D002 Option A | T001 report, receipts and decision ledger |
| Preflight/no-op nonmutation, success invariants and caller recovery | yes | contract accepted; new implementation tests pending | D002 Option A; V12, V14, V15 |
| Native and Python implementation | yes | pending | T002, T003 |
| Constraint/lifecycle regression matrix | yes | pending | T004 |
| Compatibility, build and specialist review | yes | pending | T005 |
| Durable documentation targets identified | yes | identified | requirements.md#durable-impact |
| Accepted behavior promoted | yes | pending | T006 |
| Closure and actual commit metadata | yes | pending | T006 |

## Regression Matrix

Use small generated source objects with analytic geometry and at least two
projected external references. Do not require private D600 files. Snapshot semantic
state before every operation and assert preservation beyond final solid validity.

| Case | Scenario and required assertions | Covers | Tasks | Status |
| --- | --- | --- | --- | --- |
| V01 | Two projected circles/arcs with dependent constraints; change the first reference to a different Edge selector, leaving the second in place. Assert new projection, old native ID, both orders, roles, flags and solvability. Include a selector change within the same source and an allowed different source object. | R1 AC1, AC3; R2 AC1–AC3; R4 AC4 | T002, T003, T004 | pending |
| V02 | Line/arc orientation changes, nonidentity placement and centre/start/end constraints, signed point-to-line Distance and Tangent orientation. Preserve exact Constraint.Orientation and geometric side through two ordinary recomputes, undo/redo and reopen, or reject before mutation. | R2 AC2, AC4; R3 AC2 | T001, T002, T004 | pending |
| V03 | Axis/internal/out-of-range indices, extreme integers, bool/type errors, missing/null/disallowed/cyclic source, missing/malformed selector, Frozen/Missing/Detached projections, unsupported curve, Intersection/Both modes, grouped projections, type change, degeneration and duplicate destination. Unsupported states/modes reject even for the current source. Assert exceptions and unchanged model/metadata. | R3 AC1, AC2, AC4; R1 AC2 | T002, T003, T004 | pending |
| V04 | Otherwise admitted exact same-source request is a no-op on a clean sketch, without notifications or dirty/journal change. Preparation failures preserve pre-call state. Unexpected apply failures propagate without implicit retry/commit/abort and require caller recovery; do not assert operation-local full restoration. | R3 AC3, AC4, AC5 | T002, T004 | pending |
| V05 | A transaction contains an earlier unrelated or same-property edit. Preflight failure preserves it. After an unexpected apply error, assert the transaction remains caller-owned; explicit caller abort restores the whole transaction including the earlier edit. Separately use a dedicated per-operation transaction. No hidden commit/abort/save/recompute. | R3 AC3; R4 AC1, AC2 | T002, T004 | pending |
| V06 | Commit, undo and redo; compare both states and run immediate solve/edit after each replay to detect stale native maps or solver state. Recompute normally and compare dependent geometry. | R2 AC1, AC2; R4 AC2 | T004 | pending |
| V07 | Modify the new source, then the previous source, with normal recomputes. Only the expected dependent geometry follows; unrelated external projection remains bound to its own source. | R2 AC3; R4 AC4 | T002, T004 | pending |
| V08 | Save/reopen original and reattached states in separate disposable files, preserving IDs/order/flags/constraints and correct source resolution; edit the new source after reopening. Do not assume undo history persists in files. | R2 AC1–AC3; R4 AC3, AC4 | T004 | pending |
| V09 | Existing addExternal/delExternal and grouped native operation remain compatible; generated API docs/signature/errors match the implemented method and Python fixture is registered. | R1 AC2, AC3; R5 AC1, AC2 | T003, T005 | pending |
| V10 | Focused native/Python suites, pixi build, broader tests and applicable hooks; source/API specialist reviews and platform limits recorded. | R5 AC3, AC4 | T005 | pending |
| V11 | Durable API/reference examples, requirement/property coverage and closure metadata checked against actual implementation evidence. | R1 AC2; R5 AC4 | T006 | pending |
| V12 | Read-only observers record normal property/element notifications during application; intermediate link/projection state is allowed. Require no change events on preflight rejection/no-op, safe native ownership during callbacks, and coherent links/modes/Ref/IDs/dependencies on successful return. Unexpected apply exceptions must propagate rather than claim success. | R2 AC1, AC3; R3 AC5 | T001, T002, T004 | pending |
| V13 | Success/no-op/preflight rejection/unexpected apply error with no active transaction, active transaction and booked/global transaction. noRecomputes/edit mode rejects before mutation even for same-source input. After apply errors, use explicit caller abort or owned-checkpoint reload before checking subsequent solve/edit. Cross unsupported/current-source combinations with V03. | R3 AC1, AC3, AC4; R4 AC1 | T001, T002, T003, T004 | pending |
| V14 | Exact dirty/touched/enforce/recompute and transaction/undo/redo comparison for preflight rejection and no-op, including booked-with-redo and earlier same-property edits. After unexpected apply failure, record permitted metadata/redo changes and recover explicitly; do not require discarded redo restoration. Prove caller abort reverts earlier shared-transaction edits too. | R3 AC3, AC4; R4 AC1, AC2 | T001, T002, T004 | pending |
| V15 | Preserve ordered ExternalTypes including unaffected Intersection/Both neighbours. Check InList/OutList and property dependencies after success, preflight rejection, caller recovery, undo/redo and reopen; after unexpected apply failure recover before asserting graph consistency. Test old-source deletion with no retained link, and preservation when another retained link still needs it. | R2 AC1, AC3, AC5; R3 AC2, AC3; R4 AC2–AC4 | T001, T002, T004 | pending |

### Comparison Rules

Authored constraints including Orientation, IDs, roles, flags, ExternalTypes and
order require exact comparisons. Native dependency/backlink sets must match the
expected reference graph on successful/recovered boundaries. Preflight rejection
and no-op compare dirty/touched/enforce/recompute and transaction/undo/redo state,
including earlier same-property edits. Apply errors are not no-call operations:
record partial state, assert no implicit boundary manipulation and recover through
the caller transaction or checkpoint. Intermediate notification state is permitted;
observer logs and final successful/recovered snapshots serve different purposes.
Start with absolute coordinate/radius tolerance 1e-9 mm and angular tolerance
1e-10 radians on small analytic fixtures; record justification for any change
before using it for acceptance. Never loosen tolerances to hide endpoint inversion.
Use semantic Ref resolution and curves, not ZIP bytes or regenerated UUIDs alone.
Record excluded generated fields explicitly. Require consistent Missing/Sync
status after normal rebuild/recompute; do not treat them as immutable user flags.

## Validation Commands

These are planned implementation commands unless explicitly marked executed.
T001 must pin actual test names and the supported Python runner on the selected
build. Do not claim the proposed filter exercised cases until they exist and
the discovery output lists them.

Record the expected case-name manifest and counts, compare them with discovery,
and require every intended case to execute with nonzero counts. On platforms
registering individual GoogleTests, SketchObjectTest names may not match a
Sketcher-only CTest filter. Use a platform-appropriate expression derived from
`ctest -N` and native test discovery; an empty or incomplete selection is a failed
qualification, even if the runner exits successfully. Apply the same nonempty
manifest check to the public Python module.

| Command or tool | Purpose | Result | Evidence |
| --- | --- | --- | --- |
| MCP lint_spec_package for spec 004 | Spec structure and cross-artifact checks | passed, zero errors/warnings | Evidence Log |
| git -C docs diff --check | Docs whitespace | passed | Evidence Log |
| pixi run build -j 2 | Build affected application | passed for T002 native source | t002-validation.json; generated Python method remains T003 |
| build/debug/tests/Sketcher_tests_run --gtest_list_tests | Discover native case names | 162 discovered, including 13 new cases | t002-evidence/manifest.txt |
| build/debug/tests/Sketcher_tests_run --gtest_filter='SketchObjectReattach.*' | Focused native cases | 13 passed, nonzero discovery/execution matched | t002-validation.json |
| build/debug/tests/Sketcher_tests_run --gtest_filter='*' | All native Sketcher cases | 162 passed | t002-validation.json |
| Owned built FreeCAD Python runner, unittest module SketcherTests.TestSketchExternalReattachment | Public Python and persistence matrix | not run; exact invocation required in T001 | T003, T004 |
| ctest --test-dir build/debug -N | Discover platform-specific registered case names | not run; required to select exact filters | T001, T005 |
| CTest filter derived from the pinned intended/discovered case manifest | Execute every intended affected native case; require nonzero counts | not run; generic Sketcher filter is not acceptance evidence | T005 |
| pixi run test | Broader project regressions | not run | T005 |
| pre-commit run --all-files | Repository required hooks before submission | not run; implementation submission pending | T005 |

## Requirement Coverage

| Requirement | Acceptance criteria covered by plan | Executed implementation evidence | Residual risk |
| --- | --- | --- | --- |
| Requirement 1 | AC1, AC2, AC3: V01, V03, V09, V11 | D001 API boundary selected; public surface absence confirmed | New binding not implemented |
| Requirement 2 | AC1–AC5: V01, V02, V06–V08, V12, V15 | T002 native identity/orders/modes/constraints and dependency tests | Full lifecycle and Coincident solver qualification remain T004 |
| Requirement 3 | AC1–AC5: V03–V05, V12–V14 | T002 native rejection/no-op/journal and caller recovery tests | Python input validation and full lifecycle remain T003/T004 |
| Requirement 4 | AC1–AC4: V01, V05–V08, V13–V15 | T002 native caller abort/checkpoint recovery and repeated recompute | Success undo/redo/save-reopen matrix remains T004 |
| Requirement 5 | AC1, AC2, AC3, AC4: V09–V11 | T002 standard build and all 162 native Sketcher cases pass | Public compatibility/platform/review/promotion remain T003–T006 |

## Correctness Property Coverage

| Property | Covered by | Evidence | Residual risk |
| --- | --- | --- | --- |
| CP-001 | V01, V02, V06, V08, V15 | T002 native preservation tests | Full lifecycle and Coincident solver qualification remain T004 |
| CP-002 | V03–V05, V12–V14 | T002 native nonmutation and explicit caller recovery tests | Python/full matrix remains T003/T004 |
| CP-003 | V05, V06, V08, V14 | T002 native caller-abort/checkpoint recovery | Success undo/redo/save-reopen matrix remains T004 |
| CP-004 | V01, V07, V08, V15 | T002 native Ref/dependency and old-source deletion tests | Persistence/undo/redo boundaries remain T004 |
| CP-005 | V05, V09–V11, V13, V14 | Additive native path; all 162 Sketcher cases pass | Public compatibility/review remain T003–T005 |

## Agent Readiness Evidence

| Field | Evidence | Residual risk |
| --- | --- | --- |
| Scope | D001 native/Python boundary and user-approved D002 Option A; T002 native slice implemented | Public/full lifecycle qualification remains T003–T005 |
| Must-read context | canonical-context.md plus requirements/design/tasks/traceability/verification | Reconcile on branch/source changes |
| Permissions | User authorized T001/T002 and local commits | No operator-document or runtime replacement authority inferred |
| Validation signals | T001 baseline plus T002 exact 13/162-case manifests and source/build hashes | T003/T004 public/lifecycle matrix pending |
| Review | Four simulated expert design reviews; dispositions below | Native implementation and actual human maintainer review pending |
| Promotion | API .pyi and dedicated durable reference | Not yet implemented/promoted |
| Repository evidence | Direct source reads; Agent Workbench navigation | Nested docs boundary limits automated validation planning |

## Task Evidence

| Task ID | Status | Evidence | Notes |
| --- | --- | --- | --- |
| T001 | complete | t001-qualification.md; t001-validation.json; option-a-native-receipt.json | D001 resolved locally; user accepted D002 Option A; caller-abort qualification and reconciliation complete |
| T002 | complete | t002-implementation.md; t002-validation.json | Native implementation, 13 focused/162 total cases and standard build pass |
| T003 | pending | none | No binding edits |
| T004 | pending | T002 native recovery baseline | Full lifecycle and Coincident solver disposition pending |
| T005 | pending | T002 build and native suite baseline | Broader compatibility/platform/implementation review pending |
| T006 | pending | targets identified | No promotion/closure |

## Evidence Log

| Date | Evidence | Result | Notes |
| --- | --- | --- | --- |
| 2026-11-07 | spec_creation_plan with fingerprint revalidation | 004 allocated provisionally; absent directory claimed with mkdir | Repository templates authoritative; user purpose supplied by conversation |
| 2026-11-07 | Direct source, CMake, nearby test and policy inspection | Proposed bounded design is plausible | No native runtime or implementation correctness claim |
| 2026-11-07 | lint_doc requirements.md, design.md and tasks.md | zero errors/warnings | Structural checks only |
| 2026-11-07 | Agent Workbench verification_plan | no runnable docs plan; nested docs repository not traversed | Use explicit lifecycle lint and local link/whitespace checks |
| 2026-11-07 | lint_spec_package for docs/specs/004-sketcher-external-reference-reattachment | zero errors, warnings and informational findings | Eight package artifacts; structural result only |
| 2026-11-07 | active_spec_preflight for spec 004, T001 | Package selected; six pending tasks and D001/D002 surfaced | Context readiness does not authorize or prove implementation |
| 2026-11-07 | git -C docs diff --check and bounded Python Markdown/frontmatter/target/anchor check | Passed; nine Markdown files, 47 local links, zero errors | Includes eight package documents and index; no source tests needed for authoring |
| 2026-11-07 | MoE remediation and independent transaction/test spec re-review | F001–F008 addressed in the specification; re-review found no remaining substantive contradictions in those contracts | Native mechanisms and all six implementation tasks remain unqualified/pending |
| 2026-11-07 | Revised lint_spec_package | zero errors, warnings and informational findings | Specification structure only; D001/D002 remain open |
| 2026-11-07 | Revised bounded Markdown/link/criteria/case/task/disposition checks and git -C docs diff --check | Eight Markdown files, 41 local links, new AC/case mappings, six pending tasks and eight dispositions checked; zero errors | Documentation-only validation; no source build or native runtime test |
| 2026-11-07 | T001 native/API qualification | Seven standalone observations, six focused existing native cases, five existing Python cases and one generated arc save/reopen fixture passed | t001-validation.json binds receipts; these do not establish a new implementation |
| 2026-11-07 | D002 candidate review | Actual mixed-state notifications, touched-state residue and lost redo history disqualify inspected guards/setter restoration | Contract-direction question pending; strict draft acceptance remains unchanged |
| 2026-11-07 | User D002 decision | Explicitly accepted Option A and continuation | Requirements/design/tasks/traceability/verification now use normal FreeCAD transaction semantics |
| 2026-11-07 | Updated native qualification | Exact eight-case discovery/execution passed, including injected application error and explicit caller abort restoring the model and earlier transaction edit | option-a-native-receipt.json; stable source/library hashes; new API still not implemented |
| 2026-11-07 | Option A reconciliation and T001 completion | Nine Markdown artifacts reconciled; package lint zero findings; 55 local links, Python syntax, exact 8/6/5-case receipts, arc fixture and evidence hashes checked | T001 complete for qualification only; T002–T006 pending |
| 2026-11-07 | T002 native implementation and guarded task completion | pixi run build -j 2, 13 focused and 162 total native Sketcher cases pass; t002-validation.json pins exact receipts/hashes | Native slice complete; Python/full lifecycle remain T003–T005, including T004 Coincident solver disposition |

## T001 Qualification Evidence

See [the qualification report](t001-qualification.md),
[aggregate receipt](t001-validation.json), and raw logs/receipts in t001-evidence/.
Exact case manifests, generated fixture recipes, source/library identities and
runner commands are pinned there. Probe discovery matches execution with nonzero
counts; the native source/library hashes stayed unchanged during the seven-case run.

Seven passing standalone tests assert existing limitations, including two mixed
observer notifications and redo count 1 -> 0 after setter-based restoration. They
are successful observations of behavior that fails the proposed strict contract.
Existing regressions and the arc fixture qualify the test setup and lifecycle
baseline, not reattachment acceptance. No fresh full build or platform claim.

D001 selects reattachExternal, the user accepted D002 Option A, and T001 is complete.
The updated eight-case native run qualifies caller abort after an injected native
application error. T002 uses the revised normal transaction contract; the superseded
atomic promises are historical evidence and no longer gate implementation.

## T002 Native Evidence

The additive native method and thirteen direct C++ cases are described in
[t002-implementation.md](t002-implementation.md).
[t002-validation.json](t002-validation.json) pins the exact commands, source/build
identities and raw receipts: thirteen focused cases and all 162 native Sketcher
cases pass; the normal `pixi run build -j 2` passes. Source/test scoped hooks and
inserted-region clang-format 23.1.0 checks pass with existing CRLF retained.

This establishes native admission/no-op nonmutation, successful return-state
preservation, direction rejection, immediate dependency updates and explicit
caller abort/checkpoint recovery. It does not complete the public Python, full
success lifecycle, cross-platform or expert-review gates. T004 must triage the
fully constrained centre-Coincident movement failure observed with both native
and legacy retargeting on this branch. Native preservation passes do not certify
that separate solver behavior.

## Scope Reconciliation Before Closure

| Target | Implemented in this spec | Coverage state | Destination | Blocks closure? | Evidence |
| --- | --- | --- | --- | --- | --- |
| Requirements 1–5, bounded API | T002 native slice | partial-blocking | T003–T006, including T004 Coincident solver disposition | yes | t002-implementation.md and t002-validation.json |
| Bulk/grouped/intersection retargeting | none | out-of-scope | Explicit rejection in durable reference | no | requirements non-goals |
| MCP and live D600 integration | none | out-of-scope | External modelling repository owns integration | no | ownership boundary |
| Historical crashes/performance | none | out-of-scope | Existing investigations remain independent | no | no causal claim |

## MoE Review Disposition

The user requested corrections after four independent simulated native Sketcher,
maintainer/API, transaction/observer and test reviews of root42192242fd/docs749b64c.
These are private design reviews, not actual FreeCAD contributor consultation.
The rows below resolve specification omissions; none establishes native feasibility
or completes an implementation task. Both decisions were open at that review
checkpoint; subsequent T001 evidence resolves D001 locally and the user resolves
D002 to Option A. Rows F001/F003 below record that explicit scope disposition.

| Finding | Specification correction | Required evidence or remaining gate | Disposition |
| --- | --- | --- | --- |
| F001 Observer-visible atomicity | Superseded by user-approved D002 Option A: normal intermediate notifications allowed; final successful state must be coherent | V12 observes notifications and return boundary; no new atomic guard | Explicitly re-scoped by user |
| F002 Constraint orientation | R2 AC2/AC4 preserve exact Orientation; unsupported reversed mappings reject | V02 signed Distance/Tangent through two recomputes and lifecycle operations | Addressed in spec; tests pending |
| F003 No-call failure equivalence | User-approved D002 confines no-call guarantees to preflight/no-op; apply errors require caller recovery and may alter dirty/redo metadata | V05/V14 and caller-abort qualification; no operation-local savepoint | Explicitly re-scoped by user |
| F004 Complete native state oracle | R2 AC1/AC5 and design include ExternalTypes, InList/OutList and property dependencies | V15 mode neighbours, graph preservation and old-source deletion | Addressed in spec; tests pending |
| F005 Unsupported-state/no-op precedence | R3 AC1/AC4 admit only supported states before no-op detection; Frozen/Missing/Detached and Both/Intersection reject | V03/V04/V13 combinations | Addressed in spec; tests pending |
| F006 Execution modes | R4 AC1 supports no-active/active/booked transactions; R3 AC1 rejects noRecomputes/edit mode | V13 immediate subsequent solve/edit and metadata checks | Addressed in spec; tests pending |
| F007 Runner qualification | Exact discovered/intended manifests and nonzero executed counts replace generic filter assumptions | T001 pinned native/Python baseline receipts; new API cases remain V09/V10 | Baseline runner qualification complete |
| F008 Shared-worktree workflow | Tasks/design/context keep the current branch and isolate PR preparation in another worktree | Preserve other-agent work; select only feature paths for any authorized commit | Addressed in spec |

## Manual Or External Verification

No manual runtime verification, public maintainer agreement or external CI result
has been obtained for this feature. Human API/implementation review is required
before public preparation. Follow current CONTRIBUTING.md and AI_POLICY.md.
The simulated MoE review and this disposition do not replace those human gates.
The user's D002 acceptance is a local scope decision, not upstream maintainer approval.

## Residual Risks

- Endpoint correspondence and reference-group bookkeeping are the main native risks.
- Reentrant property observers require careful notification-boundary review.
- Matching projection type does not guarantee global solver consistency.
- No Windows/macOS or old-document qualification has run.
- Build/install/live runtime state is independent of source/spec state.

## Durable Promotion And Cleanup

| Spec content | Durable destination or deferral | Status | Evidence |
| --- | --- | --- | --- |
| Public signature, errors and indexing | src/Mod/Sketcher/App/SketchObject.pyi | pending | T003, T006 |
| Accepted behavior, rationale, transaction and validation examples | docs/reference/sketcher-external-reference-reattachment.md | pending | T006 |
| Implementation evidence and package disposition | docs/history/spec-closure-log.md; docs/history/spec-archive-index.md | pending | T006 |
| Unsupported broader retargeting | Explicit limits in durable reference; no expansion committed | planned | Non-goals |

### Spec Cleanup Decision

- **Cleanup action:** keep active.
- **Reason:** draft implementation planning; no accepted implementation evidence.
- **Final spec commit:** pending.
- **Closure log path:** docs/history/spec-closure-log.md.
- **Closure log entry updated:** no.
- **Closure cleanup commit:** pending.
- **Active indexes updated:** creation row added; final disposition pending.
- **Durable docs linked back to evidence where useful:** no.
- **Residual spec-only content:** proposed contract and design; local-developer owns promotion in T006.

## Ship Or Closure Risk

- **Risk level:** medium.
- **Breaking change:** no intentional break; existing native behavior must be checked.
- **Blast radius checked:** source inspection only.
- **Rollback path:** caller-owned transaction abort or owned checkpoint recovery; no operation-local savepoint or redo restoration promise.
- **Requires human review:** yes.
- **Release notes needed:** assess at T005 under current project policy.
- **Follow-up issue or spec needed:** no additional feature scope committed.

## Readiness Decision

- **Ready to implement:** yes for T003 after fresh task context; T001/T002 are complete for their bounded slices under user-approved Option A.
- **Ready to validate:** native T002 validated; public and full lifecycle implementation/qualification remain T003–T005.
- **Ready for promotion:** no.
- **Ready for release:** no.
- **Ready for closure:** no.
- **Ready to archive:** no.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Canonical context](canonical-context.md)
