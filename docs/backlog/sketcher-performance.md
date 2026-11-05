---
title: Sketcher performance follow-up investigations
doc_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Sketcher Performance Follow-Ups

The [backlog index](README.md) owns IDs/status/priority. This document defines
bounded investigation scope, not approved implementation. Current behaviour and
capture guidance are in the [diagnostics reference](../reference/sketcher-performance-diagnostics.md)
and [investigation runbook](../runbooks/sketcher-performance-investigation.md). Historical evidence comes from
[T006](../specs/000-sketcher-diagnosis-performance/remaining-cost-decisions.md) and
its aggregate [attribution](../specs/000-sketcher-diagnosis-performance/remaining-cost-attribution.json).
Private fixtures and raw geometry remain outside Git. Use immutable copies of
operator models. Indicative repeated timings suffice; correctness remains strict.

## B001 QR method comparison

**Owner:** local-developer. **Next action:** specify a disposable dense/sparse
comparison using existing `Sketch::setSketchAutoAlgo`/`setQRAlgorithm` plumbing;
keep shipped defaults, pivot thresholds, tolerances and launch policy unchanged.

QR compute intervals cover about 64–85% of retained complex-sketch root execute,
and 40–59% on the updated large sketches. This is a substantial opportunity
window, not a forecast that sparse QR will remove it. Current automatic selection
uses diagnostic unknown count before rank subtraction and a default threshold
of 1000; all sampled sketches choose dense. The source documents rank-sensitivity
risk. A nearby comment mentioning 200 is not the configured threshold.

**Exit evidence:** frozen hashes/settings; warmups and repeated comparable timings;
both J/JT ranks, diagnostic IDs/DoF, dependent-geometry behaviour, geometry/shape
and undo/move/external/reference tests. Include aligned-slot, symmetric and nearly
rank-deficient synthetic cases. An observed mismatch rejects that configuration;
never trade correctness for speed. A separate implementation spec is required
before changing defaults, backend selection or numerical algorithms.

## B002 Async payload copying

**Owner:** local-developer. **Next action:** isolate copies/allocations within
async dispatch before deciding whether to prototype reference/shared ownership.

The current call passes J, the constraint map and diagnostic-parameter pointer
vector as lvalues to `std::async`, which stores decayed payload copies. The worker
reads them through const references. Observed dispatch covers roughly 0.4–1.9%
of large-sketch execute and includes launch/scheduling as well as copies. Only
payload copying within this span is bounded by that span; other matrix/layout
copies are not isolated. No guaranteed saving is established.

**Exit evidence:** an isolated copy/launch breakdown and a demonstrable benefit
on representative models. Any later prototype must preserve automatic/deferred
policy, wait-only exception semantics, lifetime on exceptional future destruction,
worker silence and the join-before-conflict-analysis barrier. Never pass borrowed
references past owner lifetime or mutable diagnostic redundancy work. Reuse T003's
async/deferred/exception tests and the full correctness matrix.

## B003 Geometry-key refresh

**Owner:** local-developer. **Revisit trigger:** this phase becomes material in a
measured workflow or a notification consumer is identified as expensive.

Explicit geometry acceptance is only about 0.02–0.06% on the large sampled sketches.
Its duration includes validation/notifications, not merely filling the type-key
vector. External-geometry rebuilding contains separate necessary work; its whole
cost must not be advertised as removable metadata overhead.

A key-equality fast path must still repair independent invalid-index/geometry flags
and preserve required before/change/value/diagnostic notification effects. Geometry
keys alone do not establish valid constraint input. **Exit evidence:** the C-001/
C-002 tests, internal/external invalid-to-repaired transitions, touched/observer
contract, transactions and immediate undo/solve/move all pass. No notification
suppression or early return is proposed in the current package.

## B004 Structural reuse

**Owner:** local-developer. **Revisit trigger:** profile a repeated interactive
edit and identify reusable symbolic structure separately from numeric derivatives.

Repeated dimensions/ranks on unchanged forced recompute are not a cache validity
proof. The numeric Jacobian and rank depend on current geometry and values.
Potential benefit is unknown; Jacobian assembly is about 1.6–3.4% on the sampled
large sketches, while avoiding a full diagnosis would affect a different, much
larger region and change the current one-authoritative-setup contract.

**Exit evidence:** a reviewed invalidation design for topology, geometry values,
constraint values/types/indices/order/active/driving state, external links,
restoration, undo/redo, move, solver preferences and exceptional/nested observers.
Separate immutable symbolic ownership from parameter pointer lifetimes; always
refresh numeric data where required. No reuse of stale diagnostics/Jacobians.
A separate spec must explicitly revise any one-setup invariant before caching
could replace full setup. No cache is implemented or selected now.

## B005 Sparse assembly

**Owner:** local-developer. **Revisit trigger:** measure density, assembly cost,
conversion or memory pressure on a larger representative fixture.

Current code first allocates dense J and evaluates driving-equation gradients
against diagnostic parameters; the sparse QR path subsequently converts with
`sparseView`. All sampled cases use dense QR. Assembly is only about 1.6–3.4%
of the large-sketch execute; one matrix payload is about 0.89–4.13 MB, not total
process/QR workspace memory. Current evidence does not justify a broad rewrite.

**Exit evidence:** actual structural sparsity, derivative/mapping and memory
measurements, a small equivalence prototype, and unchanged tags/row mapping,
driven filtering, ranks/diagnostics and dense fallback. A numeric zero at one
configuration is not proof of structural absence. Preserve dense fallback and
separate symbolic pattern work from numeric updates. No saving is forecast yet.

## B006 Mutation stall replay

**Owner:** local-developer. **Blocking evidence:** verify the immediate pre-mutation
checkpoint or a complete, validated reconstruction chain plus exact request.

The original saved before-repair checkpoint has been recovered and matches
SHA-256 `5bb891ab395ae8f01fec9bf5eae313a49db8b13f43ad92a79ce07fde47dcda6d`
(97 geometry/226 constraints in Sketch004). It precedes intervening edits and is
not established as the immediate pre-symmetry revision. Live replay receipts have
set-driving/tangent-removal work; coordination describes further perpendicular/
vertical changes and a symmetry operation eventually completing around 74 seconds.
Do not substitute today's 104/247 master or replay an already committed constraint.

A separate later Equality (`MatchingShoulderFlats`) has a reported 156-second native
solve envelope and a post-commit checkpoint. That envelope includes setup and
numerical work; it does not identify which caused the delay. Its exact precursor
and phase attribution are still missing. It must remain a distinct case.

**Exit evidence:** immutable input hash, exact geometry/constraint identities,
operation arguments/order, driver/reference values, source/build/preferences,
terminal operation outcome and pre/post audits. Reconcile prior timeout outcomes
read-only; never retry a live unknown/committed mutation. Then use a disposable
native replay with separate setup/QR/redundancy/numerical phases, elapsed timeout
handling and geometry/rank checks. No live replay or operator mutation is authorized
by this backlog entry. Until then both historical mutation cases remain unverified.

## B007 Shape construction

**Owner:** local-developer. **Next action:** separate wire/face conversion, internal
face generation and downstream document propagation on the updated snapshot.

Wiring/auxiliary outlines have untraced medians about 15.5/13.3 ms; their traced
shape spans cover about 81% of root execute, whereas setup covers only 13–14%.
Those are smaller absolute operations than the heavy diagnosis cases. Investigate
this distinct profile before assuming all sketch cost is QR or changing geometry.

**Exit evidence:** subphase attribution and an aggregate downstream workload,
with unchanged wire/face topology, internal-face version semantics, shape validity,
placement, dependent Pad/Pocket output and edit/undo/external/reference behaviour.
The observed shape span is an opportunity window, not an expected whole-document
saving. Any caching/elision proposal needs its own invalidation and compatibility
spec. No shape shortcut is implemented here.

## B008 External observer regression

**Owner:** local-developer. **Next action:** design a diagnostic-notification
observer that edits ExternalGeo or ExternalGeometry through a normal property/API
notification and compare legacy/candidate post-operation behaviour.

T007's independent source review found no blocking defect. It noted that the
refresh branch names external properties but observer-driven regressions directly
cover Constraints and Geometry only; the external-index repair test occurs outside
this notification. This is a nonblocking local coverage request, not an observed
model failure or a claim of universal addon compatibility.

**Exit evidence:** verify the proper external projection/index state, preserved
constraint identity/diagnostics, required fresh setup and absence of stale deferral
on the following normal execute. Include a normal external-link update and
appropriate cache rebuilding; a diagnostic touch alone is not a property setter.
If the test finds a divergence, route a bounded correctness fix before upstream
submission. Existing Linux/private-model/GUI evidence is retained as bounded,
not advertised as this missing regression. No production change is started here.
