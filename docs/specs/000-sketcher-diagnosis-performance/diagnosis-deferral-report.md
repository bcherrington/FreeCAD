---
title: Sketcher diagnosis deferral implementation evidence
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# T004 Diagnosis Deferral

## Result And Scope

T004 implements C-001/C-002 from the
[timing and invalidation contract](timing-invalidation-contract.md). An eligible
ordinary sketch execute performs one authoritative setup, replacing the two
setups recorded in T001/T003. Normal index/geometry validation still runs.
The public solve signature and FCStd properties are unchanged.

The instrumentation-only baseline was committed as root `9bc0eb91ff` and docs
`88b3ce6` before implementation. T004 does not establish elapsed performance,
tracing-off overhead budgets or broad GUI acceptance; these remain T005.

## Implementation

`SketchObject::execute()` creates a stack-owned one-shot context only around its
explicit geometry acceptance, after external-geometry rebuild. Eligibility excludes
restoration, transaction replay, managed/internal operations and `noRecomputes`.
A solver-preference preflight preserves the legacy invalid-preference path.
`onConstraintsChanged()` consumes the request before recursive work, executes its
existing unmanaged index and geometry validation, then defers only the final setup
if validation succeeds. Unexpected nested callbacks cancel the deferral.

The public `solve(bool)` forwards to private `solveInternal(..., false)`. Only the
execute with a confirmed deferral requests the diagnostic handoff. Its authoritative
setup retrieves diagnostics; the original conditional Constraints touch occurs
under an internal-transaction guard before status selection and numerical solving.
This preserves notification counts, diagnostic IDs, touched state and final outcomes
with the intermediate ordering explicitly permitted by C-002.

Input edits made by diagnostic observers need the legacy second setup. A scoped
before-change connection detects notified Geometry/Constraints/external-geometry
edits. The refresh validates indices, repairs changed geometry keys and sets up
again. Tests cover replacing redundant constraints and repairing a Radius-on-Line
input to a Circle, retaining the newly valid Radius constraint. Notification-free
raw C++ mutation is outside this property/API invalidation contract.

Acceptance exceptions restore the omitted setup while preserving the original
exception. Late invalid solver preferences restore diagnosis before the existing
error return. A Base exception in deferred setup or diagnostic notification retains
the legacy recovery attempt; unrelated exceptions still propagate. Scope restoration
is verified by a subsequent normal execute. These correctness fallbacks deliberately
retain additional setups; the invariant applies to the ordinary eligible path.

## Count Evidence

Native tracing test hooks count actual setup spans. An independent Linux/ELF
interposer verifies execution/setup vectors with tracing disabled and modules bound
to the selected build, as established in T001.

| Operation | Baseline setups | Candidate setups | Interpretation |
| --- | --- | --- | --- |
| Clean no-op | 0 | 0 | No sketch execute |
| Forced synthetic execute | 2 | 1 | One target execute |
| Forced frozen master execute | 2 | 1 | One target execute |
| Forced frozen plate-profile execute | 2 | 1 | One target execute |
| Sixteen scalar insertions plus recompute | 18 | 17 | Sixteen required scalar setups retained |
| List insertion plus checked explicit solve and recompute | 3 | 2 | Explicit solve retained |
| Edit-mode noRecomputes explicit recompute | 2 | 2 | Legacy path retained |
| Direct solve / Constraints setter / touch | 1 each | 1 each | Immediate path retained |

Synthetic/scalar/list count checks used two warmups plus one A/A pair each, with
zero rejected samples. This is an execution/correctness check, not T005's matched
ten-pair latency protocol. The baseline runner now accepts `--configuration
candidate`; default `baseline` retains the original expectations. The frozen
[workload manifest](workloads.json) remains the baseline authority.

## Behavioural Regressions

The native suite adds 12 recompute cases and one overconstraint case to T003's
136 tests, and updates the original recompute/count and trace assertions. Coverage
includes clean/ordinary recompute, edit-mode refresh, direct property/API changes,
nested callbacks, Base and standard acceptance exceptions, early/late invalid
solver preferences, diagnostic observer edits/errors and internal/external index
valid-invalid-repaired transitions. External repair verifies the orphan constraint
is visible and consumed again, with 3 DoF rather than a falsely unconstrained solve.

The diagnostic observer fixtures retain raw diagnostic IDs and touched-state
expectations. Their approved sequences are:

| Fixture | Baseline | Candidate |
| --- | --- | --- |
| Healthy horizontal line | BCPBCPBCPS | BCPBCPBCPS |
| Redundant / conflicting / malformed | BCPBCCPS | BCPBCPCS |
| Partial redundancy | BCPBCCPBCPS | BCPBCPCBCPS |

The negative-DoF overconstraint fixture independently verifies the conditional
touch and candidate sequence. B/C/P/S definitions and baseline C-bit samples
remain in C-002. The known GUI solver-info consumer reads `signalSolverUpdate`;
custom consumers of earlier stale intermediate diagnostics remain the compatibility
limit documented there. Disposable GUI panel/failure-recovery checks remain T005.

Three Python regressions exercise scalar/list transaction undo with restored geometry,
undo/redo immediately followed by solve/move without document recompute, and driving/
reference transitions with dimensional values, DoF and shape checks. Existing external
orientation/migration tests run in the same solver suite.

## Frozen Model Comparison

The frozen private model has SHA-256
`55a0b21e57c979fe9e53dd89c073c47b2718257e7e7b10298375379094e3d4eb`.
A disposable copy is opened in task-owned FreeCADCmd profiles. Separate fresh
documents exercise initial loaded state, forced master/profile recompute and
thickness 0.8 -> 0.81 -> 0.8. The no-op window after initial load is used only
for state comparison, not a fresh-load execution-count or latency claim.

Snapshots compare every sketch's geometry type/construction flag/parameters,
constraint type/references/values/name/driving/active state, DoF and diagnostic IDs,
plus expression bindings and each nonempty shape's validity, topology counts,
area, volume, bounding box and vertex coordinates matched as a multiset. Numeric tolerance is
absolute 1e-7 and relative 1e-9; identities/counts/flags are exact.
Master geometry/constraint counts remain 104/247 with 0 DoF, and plate profile
118/241 with 47 DoF. BottomPlate remains valid with one solid and 103 faces;
its thickness extent follows 0.81 and restores to 0.8. Matched Debug baseline/candidate comparisons and an additional baseline-Debug/
candidate-Release comparison each pass 153,993 scalar checks, including restore-
to-loaded-state comparisons. Coordinates and full private snapshots are retained
outside Git.

## Validation And Receipts

Commands executed on Linux on 2026-11-05, with task-owned XDG config/data/cache
and temporary directories:

- `pixi run build-release -j 2 --target Sketcher_tests_run FreeCADCmd PartDesign`: passed.
- `pixi run build-debug -j 2 --target Sketcher_tests_run FreeCADCmd PartDesign`: passed.
- `pixi run ./build/release/tests/Sketcher_tests_run`: 149/149 passed, both tracing unset and `FREECAD_SKETCHER_TRACE=1`.
- `pixi run ./build/debug/tests/Sketcher_tests_run`: 149/149 passed, tracing unset.
- FreeCADCmd `unittest` solver suite in each build: 27 run, 26 passed, one existing secant-support skip (PR 9044), zero failures/errors. Native module paths are asserted inside the selected build.
- Final independent candidate-count runner: four cases, two warmups plus one A/A pair per case, all vectors/reset assertions passed.
- Private frozen-model snapshot/comparison scripts: baseline/candidate checks described above, with exact commands and module paths retained in receipts.
- Focused pre-commit on all seven changed C++/Python production/test files passed; profiling helpers passed Black (100 columns) and Python parsing. Existing CRLF source and LF tests were preserved.
- Package lint: zero errors, the same two documented EARS/canonical-context advisories. Local Markdown links/anchors, JSON parsing and CRLF-aware whitespace checks passed.

Focused hooks were used to avoid reformatting unrelated repository files. Existing
OpenCASCADE deprecation and linker-option warnings did not fail either build.
Headless model loads report unavailable GUI geometry-extension restoration in both
configurations; the compared modelling state remains valid. GUI restoration itself
is not certified by these App-only checks.

Private evidence lives in the crash-coordination directory under
`sketch-performance-static-analysis/T004-diagnosis-deferral/`, including source
patches/hashes, build logs, native XML, Python results, module identities, counter
receipts and snapshot scripts/results. Earlier failed probes are retained with
the passing results: the external test initially selected an edge projected to a
point, Python moveGeometry was incorrectly assumed to return integer zero, and
the private snapshot initially used incorrect Python Constraint attribute casing.
Cross-build vertex arrays also required tolerance-aware multiset matching: tiny
rounding changes reorder symmetric vertices under exact lexicographic sorting.
These fixture/harness mistakes were corrected; no failed acceptance is hidden.

## Remaining Work

T005 must compare matching optimized configurations, verify every sample's actual
execution/reset vectors, apply the unchanged latency/overhead confidence gates and
exercise disposable GUI diagnostics/recovery. No speedup percentage or absence of
regression is inferred from the removed setup or these correctness runs. Cross-platform
builds were not performed. T006 attributes remaining costs after measurement; no
additional solver optimization is included here.
