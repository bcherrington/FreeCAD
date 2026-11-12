---
title: Sketcher external reference reattachment verification
doc_type: spec
artifact_type: verification
status: archived
owner: local-developer
last_reviewed: 2026-11-08
---

# Verification

## Scope

Requirements 1–5, CP-001–CP-005 and T001–T006 are implemented and locally qualified.
[Completion validation](completion-validation.json) pins commands, nonempty case
manifests, source/build hashes and raw receipts. [T004 qualification](t004-qualification.md)
explains lifecycle acceptance and the separately committed solver correction.
[Historical planning verification](planning-verification.md) records earlier stages;
its pending/unimplemented statements do not describe the final source.

No operator CAD, external MCP delivery or upstream release is certified. Current
behavior is promoted to the [durable reference](../../reference/sketcher-external-reference-reattachment.md)
and generated API help. External delivery work is owned by B014.

## Quality Gates

| Gate | Status | Evidence |
| --- | --- | --- |
| D001/D002 contract | complete locally | T001 qualification; explicit user Option A decision |
| Native/Python implementation | complete | SketchObject.h/External.cpp/PyImp.cpp/.pyi; registered native/Python fixtures |
| Preservation and lifecycle | passed | 13 native feature cases; 16 public cases under three solvers; T004 report |
| Existing Sketcher regressions | passed | 163 native cases; full TestSketcherApp 113 run, one existing secant skip |
| Build | passed | pixi run build -j 2, retained build receipts |
| Broad repository check | executed with owned failures | 2434 registered: 2422 passed, two failed, two skipped, eight disabled; B012/B013 |
| Specialist implementation review | findings addressed | Native, API, transaction and test reviews; durable private delivery review |
| Documentation promotion | implemented | Generated help, durable reference, review and owned follow-ups |
| Upstream human/platform acceptance | external | B014; no upstream approval or cross-platform CI claim |

## Regression Matrix

The following mappings are bounded acceptance evidence, not claims that every
possible model, observer, solver input or platform has been certified.

| Case | Final coverage | Evidence |
| --- | --- | --- |
| V01 | Two-reference selected-only circle/arc replacement; IDs/orders/constraints and explicit different-source operation | Native CirclesPreserveIdentityOrderModesAndConstraints; public scalar/coincidence/arc/dependency cases |
| V02 | Exact Distance/Tangent Orientation; geometric side; arc centre/start/end roles; nonidentity placement; recompute/replay/reopen; reversed/ambiguous rejection | Native SignedLineKeepsOrientationThroughRecompute and ArcKeepsParameterizedEndpointRoles; public tangent, endpoint and placement cases |
| V03 | Index extremes/bool/type/arity; absent/cyclic/cross-doc source; selector/type/degenerate/grouped/duplicate rejection; unsupported flags/modes/execution state | Native rejection cases and public boundary/mode/vertex-sibling cases |
| V04 | Exact no-op and rejection nonmutation; propagated unexpected application error | Native Snapshot oracles, observer and caller recovery cases |
| V05 | Dedicated/shared transactions; earlier same-property and label edits; whole caller abort | Native caller abort/checkpoint cases and public replay/no-op cases |
| V06 | Commit/undo/redo and immediate subsequent solve/edit | Native signed/recovery cases and public circle/tangent/endpoint cases |
| V07 | Source edits, independent retained reference, obsolete dependency removal and old-source deletion | Native ReplacingObjectUpdatesDependenciesAtReturn; public object propagation/dependency lifecycle |
| V08 | Original and retargeted checkpoints, reopen, IDs/roles/flags/constraints and later source edit | Native authored extension/signed/checkpoint cases; public original/arc/endpoint/tangent/dependency cases |
| V09 | Scalar generated binding/help/return/errors; legacy add/delete; unchanged vector/grouped interface | Public scalar/legacy cases, native grouped case, source/API review |
| V10 | Build, focused/full Sketcher checks, full CTest, scoped hooks and four specialist reviews | completion-validation.json and durable delivery review; B012/B013/B014 limits |
| V11 | Accepted current contract promoted and historical package disposition recorded | Durable reference, generated .pyi, history indexes, task/traceability reconciliation |
| V12 | No no-op/rejection events; ordinary success notifications and coherent final Ref/dependencies | Public observer case, native final invariants and injected errors |
| V13 | No transaction, booked and active caller transactions; mode rejection before no-op; explicit abort/checkpoint recovery | Native state/recovery oracles and public no-op/replay cases |
| V14 | Dirty/property/redo/pending/booked metadata and earlier caller edit preservation | Native Snapshot and public state oracles; caller abort removes earlier edits |
| V15 | Ordered mixed modes and dependency/backlink boundaries through success/recovery/replay/reopen | Native mixed-mode/dependency/cross-object injection cases and public mode/dependency lifecycle |

Frozen/Missing/Detached/Sync and native noRecomputes state are created in direct
C++ fixtures because the public interface has no supported state mutator. The
Python binding delegates directly to that same admission path; no read-only
ExternalGeometry/ExternalGeo property assignment is used by public tests.

### Comparison Rules

IDs, ordered references/roles, mode vectors, constraint fields and native
Orientation are exact. Analytic projected coordinates use 1e-9 mm; solved internal
coordinates retain the existing T001/native-test 1e-7 mm bound for nonlinear solver
tolerances. No tolerance accepts endpoint inversion or missing 40 mm motion.
Preflight/no-op compare model and journal state. Apply-error cases recover first
through the caller boundary; they do not claim operation-local restoration.

## Validation Commands

| Command | Result | Receipt |
| --- | --- | --- |
| pixi run build -j 2 | passed | completion-evidence/build.log.gz; binding-build.log.gz |
| build/debug/tests/Sketcher_tests_run --gtest_list_tests | 163 discovered, exact executed manifest matches | completion-evidence/native-manifest.txt |
| owned-profile Sketcher_tests_run --gtest_filter='*' | 163 passed, zero disabled/errors | completion-evidence/native.xml |
| FreeCADCmd --safe-mode with owned cfgs and public-runner.py | 16 cases x solver 0/1/2 = 48 passes, no skips | completion-evidence/public-matrix.json |
| same owned Cmd runner, full TestSketcherApp module | 113 run, zero failures/errors, one existing skip | completion-evidence/app.json |
| owned-profile pixi run test-debug -j 2 --output-on-failure | 2434 registered; 2422 pass/two fail/two skip/eight disabled | completion-evidence/ctest-final.txt.gz |
| original pixi run test | executed before final fixes/profile isolation; not final acceptance | completion-evidence/ctest-baseline.txt.gz |
| scoped pre-commit and inserted-region clang-format 23.1.0 | passed; legacy files not broadly reformatted | completion evidence hooks/format records |

The alias `pixi run test` does not forward additional arguments to its dependency.
The final full run therefore calls its actual test-debug task explicitly. An
intermediate serial restart is not counted as a completed validation. The two
runtime-skipped CTest entries and eight disabled entries are listed separately
in the receipt; skipped tests are not counted as passes.

## Requirement Coverage

| Requirement | Executed implementation evidence | Coverage state | Residual destination |
| --- | --- | --- | --- |
| Requirement 1 AC1–AC3 | Generated scalar binding/help, registered public surface and boundary tests | complete | B014 upstream delivery only |
| Requirement 2 AC1–AC5 | Native IDs/orders/modes/Orientation, endpoint/centre lifecycle, Ref/dependency tests | complete | none for accepted local scope |
| Requirement 3 AC1–AC5 | Preflight/no-op oracles, unsupported-state tests, ordinary notifications and injected error recovery | complete | none for accepted local scope |
| Requirement 4 AC1–AC4 | Caller abort/checkpoint, undo/redo, save/reopen and source propagation | complete | none for accepted local scope |
| Requirement 5 AC1–AC4 | Existing regression checks, full check/failure dispositions, specialist review and durable promotion | complete | B012/B013 whole-project failures; B014 external CI/human delivery |

## Correctness Property Coverage

| Property | Evidence | Coverage state |
| --- | --- | --- |
| CP-001 | Native exact preservation and public role/side lifecycle | complete |
| CP-002 | Native Snapshot/public state oracles and explicit recovery | complete |
| CP-003 | Transaction replay, persistence and checkpoint tests | complete |
| CP-004 | Immediate Ref/dependencies and subsequent source propagation | complete |
| CP-005 | Additive interface, existing suites and reviewed shared corrections | complete |

## Agent Readiness Evidence

User authorized implementation/completion and local commits. Shared branch and
unrelated GUI/spec003 work are preserved. Agent Workbench supplied bounded
navigation/plans; direct source reads and executed receipts establish correctness.
No missing upstream approval is treated as permission to publish.

## Task Evidence

| Task | Final evidence |
| --- | --- |
| T001 | t001-qualification.md; t001-validation.json; user Option A decision |
| T002 | t002-implementation.md; t002-validation.json, extended by T004 |
| T003 | Generated binding, public 16x3-case receipt and CMake script registration |
| T004 | t004-qualification.md; strong native/public lifecycle and GCS before/after regression |
| T005 | completion-validation.json; four addressed reviews; full CTest dispositions |
| T006 | Durable reference/help, owned follow-ups and actual history commit metadata |

## Evidence Log

Earlier authoring and T001/T002 observations are preserved in planning-verification.md
and their named reports/receipts. Final 2026-11-08 evidence supersedes provisional
claims that the API or lifecycle qualification was still absent. The separate
solver fix commit is 42144bc1e9. Final implementation/archive commit references
are resolved in the closure log after their commits actually exist.

## T001 Qualification Evidence

The eight native qualification probes, six prior native cases, five prior Python
cases and generated arc fixture established the implementation boundary. Their
raw evidence is historical, not a certificate for a later binary.

## T002 Native Evidence

T002 delivered the native helper and thirteen regressions. T004 resolved its
recorded solver finding and extended exact modes/Orientation/dependency lifecycle
checks. The final count is 163 native tests after adding the GCS regression.

## Scope Reconciliation Before Closure

Accepted single-reference behavior is implemented and promoted. Bulk/grouped,
intersection, cross-document and MCP integration work remain explicit non-goals.
The method preserves ordinary transaction boundaries and does not add generic
atomic infrastructure. Source and documentation boundaries are recorded in the
completion receipt and durable reference. B012/B013 are outside-module CTest
failures; B014 owns upstream/human/platform delivery. None is a hidden unfinished
piece of the accepted API.

## MoE Review Disposition

Initial F001–F008 spec findings are preserved in planning-verification.md. Actual
implementation review findings and their fixes are recorded in
[the durable private review](../../reviews/sketcher-reattachment-delivery-review.md).
Native, API, transaction and test specialists found no remaining blocker in their
reviewed scopes after corrections. No actual FreeCAD maintainer acceptance is claimed.

## Promotion Review

The current contract, examples and recovery boundaries are promoted to
[the reference](../../reference/sketcher-external-reference-reattachment.md) and
SketchObject.pyi. Review/limits and owned future work are durable. Historical
qualification logs remain here as audit evidence, not sole current authority.

## Residual Risks

The full repository is not green: B012/B013 own the Measurement handler-null and
Part loft-selector failures. Eight tests are disabled and two skip at runtime;
one existing public secant test is skipped. These counts are explicit rather
than inferred passes. B014 owns clean-base CI, Windows/macOS and human upstream
API review. The accepted local operation has no unimplemented requirement routed
into those items, and no release/upstream acceptance is inferred.

## Readiness Decision

Local implementation, validation and promotion complete. Package retained as
history; actual cleanup commit metadata is reconciled in the closure log and
completion receipt. T001–T006 are complete. Whole-project green CI and
upstream release acceptance are not claimed; B012–B014 own those delivery gates.

## Related Artifacts

- [Requirements](requirements.md), [Design](design.md), [Tasks](tasks.md)
- [Traceability](traceability.md), [Canonical context](canonical-context.md)
- [T004 qualification](t004-qualification.md), [Completion receipt](completion-validation.json)
