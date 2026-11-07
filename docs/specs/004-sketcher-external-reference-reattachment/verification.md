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
plan plus a spec-authoring record. No native implementation, new regression,
runtime activation or live model retargeting has been performed by this package.

## Quality Gates

| Gate | Required? | Status | Evidence |
| --- | --- | --- | --- |
| Requirements and proposed design present | yes | drafted | requirements.md; design.md |
| D001 and D002 qualified before implementation | yes | pending | T001 |
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
| V02 | Line and arc orientation changes, nonidentity sketch/source placement, and centre/start/end constraints. Preserve proven endpoint correspondence; reject ambiguous mapping with no state change. | R2 AC4; R3 AC2 | T001, T002, T004 | pending |
| V03 | Axis/internal/out-of-range indices, extreme integers, bool/type errors, missing/null/disallowed/cyclic source, missing/malformed selector, unsupported curve, intersections, grouped projections, type change, degeneration and duplicate destination. Assert exceptions and no partial authored changes. | R3 AC1, AC2; R1 AC2 | T002, T003, T004 | pending |
| V04 | Exact same source is a no-op on a clean saved sketch; inject projection/preparation and application failures through established native test seams. Assert ID/order/constraint preservation and usable lookups/solve afterward. | R3 AC3, AC4 | T002, T004 | pending |
| V05 | Transaction contains an unrelated earlier edit. Failed reattachment preserves that edit; successful reattachment plus explicit abort restores the transaction. Assert no hidden top-level commit/abort, save or whole-document recompute. | R3 AC3; R4 AC1, AC2 | T002, T004 | pending |
| V06 | Commit, undo and redo; compare both states and run immediate solve/edit after each replay to detect stale native maps or solver state. Recompute normally and compare dependent geometry. | R2 AC1, AC2; R4 AC2 | T004 | pending |
| V07 | Modify the new source, then the previous source, with normal recomputes. Only the expected dependent geometry follows; unrelated external projection remains bound to its own source. | R2 AC3; R4 AC4 | T002, T004 | pending |
| V08 | Save/reopen original and reattached states in separate disposable files, preserving IDs/order/flags/constraints and correct source resolution; edit the new source after reopening. Do not assume undo history persists in files. | R2 AC1–AC3; R4 AC3, AC4 | T004 | pending |
| V09 | Existing addExternal/delExternal and grouped native operation remain compatible; generated API docs/signature/errors match the implemented method and Python fixture is registered. | R1 AC2, AC3; R5 AC1, AC2 | T003, T005 | pending |
| V10 | Focused native/Python suites, pixi build, broader tests and applicable hooks; source/API specialist reviews and platform limits recorded. | R5 AC3, AC4 | T005 | pending |
| V11 | Durable API/reference examples, requirement/property coverage and closure metadata checked against actual implementation evidence. | R1 AC2; R5 AC4 | T006 | pending |

### Comparison Rules

Authored constraints, IDs, roles, flags and order require exact comparisons.
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

| Command or tool | Purpose | Result | Evidence |
| --- | --- | --- | --- |
| MCP lint_spec_package for spec 004 | Spec structure and cross-artifact checks | passed, zero errors/warnings | Evidence Log |
| git -C docs diff --check | Docs whitespace | passed | Evidence Log |
| pixi run build -j 2 | Build affected application and generated bindings | not run; implementation pending | T005 |
| build/debug/tests/Sketcher_tests_run --gtest_list_tests | Discover native case names | not run; qualify runner in T001 | T001 |
| build/debug/tests/Sketcher_tests_run --gtest_filter='*ExternalReattach*' | Proposed focused native cases | not run; name cases in T002 | T002, T004 |
| Owned built FreeCAD Python runner, unittest module SketcherTests.TestSketchExternalReattachment | Public Python and persistence matrix | not run; exact invocation required in T001 | T003, T004 |
| ctest --test-dir build/debug -R Sketcher --output-on-failure | Existing affected CTest targets, after inventory | not run | T005 |
| pixi run test | Broader project regressions | not run | T005 |
| pre-commit run --all-files | Repository required hooks before submission | not run; implementation submission pending | T005 |

## Requirement Coverage

| Requirement | Acceptance criteria covered by plan | Executed implementation evidence | Residual risk |
| --- | --- | --- | --- |
| Requirement 1 | AC1, AC2, AC3: V01, V03, V09, V11 | none | Proposed signature unqualified |
| Requirement 2 | AC1, AC2, AC3, AC4: V01, V02, V06–V08 | none | Native identity/orientation behavior unproven |
| Requirement 3 | AC1, AC2, AC3, AC4: V03–V05 | none | Strong exception guarantee unproven |
| Requirement 4 | AC1, AC2, AC3, AC4: V01, V05–V08 | none | Lifecycle reconstruction unproven |
| Requirement 5 | AC1, AC2, AC3, AC4: V09–V11 | none | Compatibility/platform and review evidence pending |

## Correctness Property Coverage

| Property | Covered by | Evidence | Residual risk |
| --- | --- | --- | --- |
| CP-001 | V01, V02, V06, V08 | pending | Identity preservation unverified |
| CP-002 | V03, V04, V05 | pending | Native failure restoration unverified |
| CP-003 | V05, V06, V08 | pending | Transaction/persistence unverified |
| CP-004 | V01, V07, V08 | pending | Propagation unverified |
| CP-005 | V05, V09, V10, V11 | pending | Existing behavior/review unverified |

## Agent Readiness Evidence

| Field | Evidence | Residual risk |
| --- | --- | --- |
| Scope | Requirements and non-goals; proposed native/Python boundaries | D001/D002 require T001 |
| Must-read context | canonical-context.md plus requirements/design/tasks/traceability/verification | Reconcile on branch/source changes |
| Permissions | Spec creation only; implementation requires a later user instruction | No live runtime authority inferred |
| Validation signals | V01–V11; commands above | Exact Python invocation remains T001 output |
| Review | Native exception/identity and Python API semantics | Specialist review not performed |
| Promotion | API .pyi and dedicated durable reference | Not yet implemented/promoted |
| Repository evidence | Direct source reads; Agent Workbench navigation | Nested docs boundary limits automated validation planning |

## Task Evidence

| Task ID | Status | Evidence | Notes |
| --- | --- | --- | --- |
| T001 | pending | source baseline only | Qualification not complete |
| T002 | pending | none | No native edits |
| T003 | pending | none | No binding edits |
| T004 | pending | none | No lifecycle tests |
| T005 | pending | none | No implementation build/review |
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

## Scope Reconciliation Before Closure

| Target | Implemented in this spec | Coverage state | Destination | Blocks closure? | Evidence |
| --- | --- | --- | --- | --- | --- |
| Requirements 1–5, bounded API | none | not-covered | T002–T006 | yes | planning only |
| Bulk/grouped/intersection retargeting | none | out-of-scope | Explicit rejection in durable reference | no | requirements non-goals |
| MCP and live D600 integration | none | out-of-scope | External modelling repository owns integration | no | ownership boundary |
| Historical crashes/performance | none | out-of-scope | Existing investigations remain independent | no | no causal claim |

## Manual Or External Verification

No manual runtime verification, public maintainer agreement or external CI result
has been obtained for this feature. Human API/implementation review is required
before public preparation. Follow current CONTRIBUTING.md and AI_POLICY.md.

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
- **Rollback path:** normal caller transactions plus required operation-local exception restoration.
- **Requires human review:** yes.
- **Release notes needed:** assess at T005 under current project policy.
- **Follow-up issue or spec needed:** no additional feature scope committed.

## Readiness Decision

- **Ready to implement:** T001 qualification only after user requests execution; T002 awaits D001/D002.
- **Ready to validate:** no implementation yet.
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
