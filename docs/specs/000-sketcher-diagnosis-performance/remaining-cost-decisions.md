---
title: Remaining Sketcher costs and follow-up decisions
doc_type: spec
artifact_type: research
status: archived
owner: local-developer
last_reviewed: 2026-11-05
---

# T006 Remaining Costs And Decisions

## Decision

Prioritize a bounded comparison of the existing QR methods on complex,
rank-sensitive fixtures. Investigate mutation-specific stalls only after verifying
their immediate precursor. Treat shape construction on smaller outlines as a
separate profile. Do not start a metadata, copy, reuse or sparse-assembly rewrite
on constraint count alone. The ranked proposals are owned in the local
[backlog](../../backlog/README.md), with bounded scope and exit checks in
[follow-up details](../../backlog/sketcher-performance.md).

T005 was committed as root `27d524eaaa` and docs `43e66da` before this analysis.
T006 uses the [indicative evidence policy](timing-evidence-decision.md); no strict
confidence gate or new production optimization is introduced. T007 still owns
final review, durable reference/runbook promotion and closure.

## Measurement Method And Limits

[remaining-cost-attribution.json](remaining-cost-attribution.json) records the
aggregate measurements and counters. Retained T005 and new opt-in traces use
operation-relative timestamps. Each denominator is the actual root execute span,
not the sum of nested `operation` durations. Phase duration is the union of its
intervals clipped to that root; J/JT worker overlap is counted once. Setup,
diagnosis, QR and join remain inclusive/overlapping categories and cannot be
added to produce wall time. Join measures waiting, not an extra independent QR cost.

Retained trace-on measurements are single attribution samples; they do not
establish a new before/after speedup or untraced latency. The updated model has
three tracing-off forced recomputes per sketch after a warmup, plus a separate
tracing-on process with one captured recompute per sketch. Inputs/settings are
unchanged. All measured windows count actual target execute/setup calls; full
preparation, snapshots/validation and dependent recompute occur outside the timer.
The shared host is not isolated; these are indicative observations, not guarantees.

## Retained Complex-Sketch Attribution

Percentages below are fractions of root execute in each candidate trace:

| Phase | Master | Original plate profile | Interpretation |
| --- | --- | --- | --- |
| Setup | 87.51% | 92.55% | Includes construction and diagnosis |
| Diagnosis | 83.02% | 89.47% | Nested within setup |
| QR compute union | 64.10% | 84.95% | Both workers, overlap counted once |
| Jacobian assembly | 2.19% | 1.61% | Nested within diagnosis |
| Async dispatch | 0.38% | 1.22% | Includes payload copies, launch and scheduling |
| Explicit geometry acceptance | 0.05% | 0.02% | Includes validation/notifications |
| Numerical solve | 1.66% | 1.03% | Main solve, separate from setup |
| Shape construction | 9.63% | 4.25% | Kernel shape work after solving |

Diagnosis consumes approximately 95–97% of setup in those traces. Thus the two-to-
one setup change removed a meaningful repeat of expensive work. The remaining
large cost is mostly QR-related, not the explicit geometry-key acceptance loop.
The original instrumentation-only geometry-acceptance span contained the duplicate
setup; attributing its former 46–52% to pure metadata would be incorrect.

Async dispatch bounds only the async payload-copy opportunity within that span.
It is not an isolated copy benchmark and does not bound all matrix transposes,
allocations or copies elsewhere in the solver. An idealized removal of all work
in any enclosing phase bounds that phase's possible saving; real savings may be
zero or negative. No numeric speedup is forecast for a proposed rewrite.

## Updated Model Qualification

The user-provided file being edited was never opened or modified by FreeCAD.
T006 used a further disposable copy of the already frozen snapshot with SHA-256
`527589a077dcdc27330cb1cac8a54ce14d10e47f2c5c33f7b2cef6e6bc5a5075`.
The snapshot was stable during its original copy and its hash stayed unchanged.
It is a separate fixture, not a replacement for T005's original `55a0b21e...` model
or a precursor for the historical mutation. The binary is the unchanged candidate
Sketcher module identified in T005 (`72a7a813...`).

All 13 sketches pass one-execute/one-setup checks for all 39 untraced and 13 traced
samples. Geometry type/construction/coordinates/parameters, constraint identities/
values/active/driving flags, DoF and diagnostic lists remain unchanged within
absolute 1e-7/relative 1e-9 numeric tolerance. All sketch shapes are valid and the
initial document contains no objects marked Invalid. No save, repair, constraint
edit, solver preference change or mutation replay was performed.

| Sketch | Geometry / constraints | DoF | Untraced median ms | Range ms |
| --- | --- | --- | --- | --- |
| Sketch004 | 104 / 247 | 0 | 56.886 | 53.618–57.179 |
| EffectorMountingHoleProfile | 6 / 12 | 0 | 1.743 | 1.584–1.751 |
| EffectorPlateHoleProfile | 6 / 12 | 0 | 1.858 | 1.845–2.039 |
| EffectorInnerBossHoleProfile | 3 / 6 | 0 | 4.973 | 3.205–5.306 |
| EffectorLobeHoleProfile | 2 / 4 | 0 | 1.342 | 1.097–4.013 |
| EffectorReliefProfile | 6 / 15 | 0 | 2.387 | 2.236–2.854 |
| WiringPlateOutline | 17 / 41 | 0 | 15.469 | 15.366–18.993 |
| AuxiliaryPlateProfile | 15 / 36 | 0 | 13.255 | 12.491–13.871 |
| BottomPlateHoleLayout | 20 / 45 | 0 | 4.115 | 3.769–4.130 |
| BottomPlateProfile | 72 / 158 | 14 | 61.337 | 49.496–63.342 |
| BottomPlateInnerOpeningSketch | 58 / 132 | 6 | 34.833 | 33.578–42.564 |
| ArmBracketOutline | 13 / 32 | 0 | 2.974 | 2.586–3.353 |
| ArmBracketHoleLayout | 2 / 4 | 0 | 1.206 | 1.152–1.338 |

The updated master, outer profile and inner opening have QR compute unions about
59%, 50% and 40% of root execute. Their dense effective matrices are respectively
481x481, 424x438 and 330x336, with ranks 481, 424 and 330 in both directions.
The original plate matrix is 695x742, rank 695. Default automatic selection uses
diagnostic unknown count before rank subtraction with threshold 1000, not final
DoF; the nearby 200-parameter comment is not authoritative. Every sampled case
selected dense QR. No sparse performance or equivalence claim is made.

WiringPlateOutline and AuxiliaryPlateProfile differ: shape construction covers
approximately 81% of their traced execute, while setup is only 13–14%. Their
untraced medians are about 15.5 and 13.3 ms, smaller in absolute time than the
heavy diagnosis cases. This motivates a targeted shape/propagation investigation,
not assuming QR dominates every sketch. Updated-profile timings must not be
compared to the different original plate as a claimed optimization speedup.

## Ranked Follow-Up Decisions

Owner for each item is **local-developer**; the backlog index owns status/priority.
These are proposed/deferred investigations, not permission to change defaults.

| Rank | Destination | Measured opportunity / bounded expectation | Risk and disposition |
| --- | --- | --- | --- |
| 1 | B001 QR method comparison | QR compute covers 40–85% of large-sketch traced execute; actual alternative-backend saving unknown | Proposed existing-method experiment with rank/geometry/diagnostic equivalence; no default or tolerance change |
| 2 | B006 Mutation-stall replay | Historical ~74 s symmetry / ~156 s native-solve envelope lack current phase attribution | Deferred until immediate precursor/argument chain is verified; original starting checkpoint recovered, no live retry |
| 3 | B007 Shape construction | About 81% on two smaller outlines; whole-document saving unmeasured | Proposed subphase/propagation investigation; preserve shape/topology/internal-face semantics |
| 4 | B002 Async payload copies | Dispatch about 0.4–1.9% on large sketches; copy-only saving no greater than its enclosing dispatch window | Proposed isolated copy/launch measurement first; ownership, deferred execution and exception lifetime risks block a casual reference swap |
| 5 | B003 Geometry-key refresh | Explicit acceptance about 0.02–0.06% on large sketches; pure key work is only part of that small span | Deferred for low current opportunity; preserve C-001/C-002 validation and notifications |
| 6 | B004 Structural reuse | Assembly about 1.6–3.4%; any diagnosis-wide caching benefit is unmeasured and would affect the one-setup contract | Deferred pending invalidation design and a separate spec; repeated sizes do not prove reusable numeric Jacobians/ranks |
| 7 | B005 Sparse assembly | Dense assembly is a small current fraction; sparse conversion/memory pressure not measured in these dense runs | Deferred until density/assembly/memory evidence warrants it; no broad rewrite, dense fallback retained |

The [detailed backlog](../../backlog/sketcher-performance.md) defines trigger,
validation workload and exit evidence for each item. The initial preferred next
investigation is B001, with B007 useful for smaller-outline workflows. None is
implemented by T006. Lower-ranked work is not silently discarded.

## Source Evidence

At root `27d524eaaa`, inspect these symbols (line numbers are navigation only):

- `PropertyConstraintList::acceptGeometry`: rebuilds type keys and still delivers
  before/change notifications. Index invalidity is independent of geometry keys;
  the C-001/C-002 contract remains required for any future fast path.
- `System::makeReducedJacobian`: dense zero allocation and equation/parameter
  gradient traversal, with driven filtering and constraint/tag mappings.
- `System::diagnose`: dense/sparse selection after diagnostic unknown count is
  known; two QR decompositions serve different diagnostic purposes.
- `Trace::async` and `System::identifyDependentParametersDenseQR`: the lvalue J,
  map and parameter-pointer vector become decayed async payload copies, even
  though the worker reads const references. Preserve the join before mutable
  conflict/redundancy analysis and original deferred/exception behaviour.
- `System::makeSparseQRDecomposition`: converts the already assembled dense J
  through `sparseView` and compressed storage. Selecting sparse QR is not direct
  sparse assembly. The source explicitly documents rank-sensitivity concerns.
- `SketchObject::buildShape`: separate post-solve shape phase; its small-sketch
  cost is a different target from diagnosis and needs its own subphase evidence.

## Historical Mutation Authority

The retained before-repair file was copied and verified against the replay
receipt's SHA-256 `5bb891ab395ae8f01fec9bf5eae313a49db8b13f43ad92a79ce07fde47dcda6d`.
Its Sketch004 has 97 geometries/226 constraints. This recovers the original saved
starting artifact; it is not today's 104/247 master and not proof of the immediate
pre-symmetry revision after the intervening edits. The initial latency receipt
marks live replay incomplete. Subsequent coordination records further changes
and eventual ~74-second symmetry completion. No saved immediate precursor or
fully verified reconstruction chain was established in T006.

The later MatchingShoulderFlats Equality is separate: coordination records native
append ~1.48 ms, a ~156,032 ms native solve envelope, later recompute/validation
and a post-commit checkpoint. The wrapper's solve includes setup; its label cannot
identify numerical convergence versus diagnosis as the cause. Later edited saved
snapshots are not interchangeable with its pre-operation input. Both cases remain
**unverified historical mutation workloads**, owned by B006 with concrete recovery
requirements. No unknown or already committed operation was retried.

## Validation And Handoff

Private evidence under `sketch-performance-static-analysis/T006-attribution/`
retains the updated/historical snapshot receipts, source/binary hashes, private
state snapshots, logs, driver/harness, complete phase records and interval-union
analysis. Only aggregate timings/counts are committed here. No FCStd or raw
geometry/constraint payload is added to Git.

The updated snapshot qualification passes in tracing-off/on processes; original
T005 data and its indicative acceptance remain unchanged. No production code,
solver default, tolerance, async policy or cache changed; no new application build
or unrelated test rerun is required for this documentation/analysis slice. Source
reads, aggregate calculations, backlog routing, links/JSON/whitespace and package
checks are the validation scope. Agent Workbench provided navigation context;
its verification-plan transport then closed, so validation planning used the
explicit T006 acceptance and direct source/evidence reads instead.

T006 completes attribution/selection. T007 can promote the verified diagnostic
interface and workflow, reference this owned backlog, review delivery and reconcile
commits/integration. The historical replay gap and speculative savings must remain
explicit in promoted guidance.

Final checks: all 52 new native sample vectors/states pass; all 13 sketch shapes
are valid; both frozen hashes are unchanged. An independent two-worker overlap
calculation matches the retained QR interval union. All 142 local file/anchor
links, aggregate JSON and seven backlog source/status/ID entries pass validation.
Spec Lifecycle Manager recognizes all seven backlog entries with valid planning
state and no planning diagnostics. Package lint has zero errors and two advisory
findings; source authority is kept explicit above. CRLF-aware whitespace passes,
and production/test/profiling source is unchanged from the T005 commit.
