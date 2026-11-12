---
title: Spec005 T001 workload and collection freeze
doc_type: spec
artifact_type: review
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# T001 Workload And Collection Freeze

T001 selects and preserves the inputs and rules for the mainstream CAD
investigation. It does not qualify a Release executable, measurement runner,
render endpoint or performance result. D001 is resolved by this contract;
T002 must resolve D002 before accepting any study sample.

## Frozen Artifacts

- [Workload registry](workload-manifest.json): `spec005-fixtures-20261109-v1`.
- [Measurement plan](measurement-plan.json): `spec005-mainstream-cad-screening-20261109-v1`.
- [Canonical public files](fixtures/canonical/): 17 FCStd and two STEP files,
  1,026,459 bytes in total; exact file hashes in the registry.
- [Preparation receipt](t001-fixture-preparation.json): executed sources,
  commands, semantic snapshots, isolation and output bindings.
- [Validation receipt](t001-validation.json): final hashes, reviews and task gates.

These files are the v1 prospective freeze. Preserve the original bytes if a
future amendment is needed: issue a new version/identity and review before new
samples. Generated archives include metadata and need not regenerate byte for
byte; the committed canonical files are the exact restoration source. Private
build outputs are preparation provenance, never the only fixture restore path.

## Selected Workloads

| Family | Exact selected models | Independent expectations |
| --- | --- | --- |
| W01 machined part | 80/240 mm stock; 3/10 patterned through holes | Dimensions, hole centres/radii, analytic volume and width-edit delta; first-hole reference witness |
| W02 feature history | 4/24 dependent Pocket features | Exact circle locations, valid solid, analytic cut volume and upstream-width propagation |
| W03 assemblies/tree | 4/24 linked jointed components; separate 16/512-link CAD tree | Fixed/grounded and revolute connector/placement rules; hierarchy/link identities; no images |
| W04 detailed STEP/native solids | Analytic plates with 48/192 through holes; two corresponding STEP exports | Single solid, dimensions/volume/hole count, unique top plane and safe pick point (5,5,8) |
| W05 ordinary sketch | Constrained rectangle and one circle, dependent Pad | Positive solution branch, dimensions/circle/DoF, edit/drag/constraint transition and Pad propagation |
| W06 complex sketches | Repetitive 16/256-hole constrained plates | Grid/radius/order/DoF and analytic Pad volume; explicitly sparse, not a densely coupled production-sketch claim |
| QR controls | Relational aligned capsule, symmetry, near-tangent point distances | Analytic geometry/rank/branch; near-axis symmetry and W06-small remain preparation-only controls |

All data are generated from included public numeric recipes, with explicit
LGPL-2.1-or-later creator/license metadata. The normalization helper preserves
all nonmetadata geometry archive members. The final 17 canonical FCStd files
were reopened and compared with the prepared semantic snapshots; both canonical
STEP files were read and checked. These are preparation checks on available
Debug modules, not Release/harness qualification or a performance campaign.

Full joint graph, global connectors, allowed motion, solver status and undo/reopen
checks remain mandatory in T002. Rounded snapshots and shape counts/volume alone
are not the final CAD-reference oracle. Derived feature previews and boolean,
fillet/chamfer/reference recipes must be materialized and qualified by T002.

## Scheduled Collection

| Stream | Planned cells |
| --- | --- |
| R01 parametric recompute | 8 |
| S01 sketch interaction | 6 |
| Q01 existing QR methods / B001 | 5 |
| F01 patterns, booleans, fillets and chamfers | 10 |
| A01 assembly operations | 4 |
| V01 viewport and selection | 8 |
| T01 large-model tree / B011 | 8 |
| I01 native persistence and distinct STEP import | 11 |

The 60 focused cells materialize 390 scheduled rows: 65 untimed warmups and
325 future timed attempts/arms. Five scheduled attempts or matched pairs are
required per cell, with no replenishment. Missing or invalid coverage yields
inconclusive/stop. Recovery studies have new prospective freezes and never
replace or pool original attempts. No study row has executed.

Within each stream, five seeded round-robin blocks rotate/reverse cell order;
QR arm order is counterbalanced, paired consecutively, in fresh owned processes.
Preview and acceptance have separate non-additive timings and geometry/transaction/
cancel oracles. Viewport pacing, admission/drop/coalescing and witnessed frame
semantics are frozen; a narrower render endpoint must be labeled honestly.

## Runtime, Host And Resource Boundaries

Product-source scope is authoring commit `877ebdfb3e` (docs `5fcf6b1`), with the
latest product-code change at `21903ca31f`. T001 makes no product-code change.
Timing selects the existing `build/release` directory and optimized Release
preset; its old outputs are inventory-only and ineligible. T002 must rebuild and
prove actual compiler/source/binary/Python/driver/loaded-module identities.
No new build tree or worktree was created by T001.

The selected GUI target is native Linux X11 on the Intel Mesa renderer, with
1280×720 viewport and frozen quality. Actual realization, input/style/plugin and
presentation witnesses are T002 qualifications, not inferred from the selection.
Host checks include a quiet admission window, CPU/memory/swap/pressure/power
context and declared missing sensors; host load alone does not prove causation.

Reservations are jointly feasible against the recorded inventory: 25 GiB retained
free space, at most 6 GiB combined in-place build/cache growth, 4 GiB private
study evidence and 64 MiB publication. T002 must reserve the remaining study
maxima after building and project all rows/profiles/telemetry/cleanup within
four hours total and 30 minutes per stream before launch. It may not silently
increase caps, delete old evidence or treat an overrun as a passing campaign.

Raw runtime observations, maps, environment/argv, clocks, locators and captures
are private by default, with owned permissions and a public whitelist. Public
fixture geometry does not make runtime provenance public. Preparation configuration
paths were observed inside the owned task root; executed helper snapshots and
private-output hashes substantiate that boundary. The earlier shared disposable
profile restored an assembly Boolean by value; final auxiliary preparation also
restores key absence in a fresh per-tag profile. No operator profile was selected.

## Qualification Gates Still Open

D002 includes actual Release/runtime and runner qualification; native QR selection,
scaling/rank-cutoff mapping; full joint/observer/reference oracles; preview cancel;
frame witness; telemetry overhead; and time/storage feasibility. The near-tangent
case is analytically ill-conditioned, not yet proven close to a native numerical
rank cutoff. If it does not satisfy the frozen sensitivity band, Q01 needs a
prospective fixture/plan amendment before its study freeze. No rank tolerance may
be tuned to manufacture coverage or agreement.

## Review And Failed Preparation

CAD/FreeCAD lifecycle, numerical/performance/QA and operations/process experts
reviewed the actual final artifacts and cleared blocking findings. The validation
receipt records dispositions. These are local agent reviews, not maintainer
approval. The initial scalar sketch generator timed out; it and subsequent
superseded preparation outputs are retained privately and ineligible. Batched
construction, stronger semantic checks and metadata fixes were qualified in
separate preparations. None is a measurement replacement or timing sample.
