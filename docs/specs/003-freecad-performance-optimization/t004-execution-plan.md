---
title: T004 comparison construction and retained P07 validation plan
doc_type: spec
artifact_type: execution-plan
status: approved-for-planning
owner: local-developer
last_reviewed: 2026-11-08
---

# T004 Execution Plan

## Decision And Evidence Boundary

The owner chose to retain full-resolution lazy P07 on 2026-11-08 and then
requested T004. This plan preserves that direction. Codex selects the technical
execution plan under that instruction; independent agent review is not human or
maintainer approval. No new product latency tradeoff is selected.

D001 remains the acceptance contract: first show median 500 ms/p95 1000 ms;
repeat show 50/100 ms; opening, open-plus-first-show, control and resource rules
remain unchanged. These are existing local engineering targets. The owner's
comment that under one second might be acceptable is not an exact replacement
median/tail contract. A future numeric change requires a separate dated decision
and new applicable evidence. Both T020 campaigns keep their original outcomes.
P07 remains **NOT ACCEPTED**, including its observed 857.91/1843.09 ms first show.

Resolving T022/D005 selects retention, diagnostic limits and comparison
construction. It permits T004.1 construction and qualification, not automatic
sampling or production acceptance. T004, T004.2 and T010 cannot complete while
the retained P07 fails its applicable contract. T004.1 need not wait for a P07
remedy when P07 is identical in both restore arms and attribution stays explicit.

## Source Composition

Use corrected T020 lazy/common commit
`613def1ab6bf615f82fe163e3b41489d0228acbf` as the construction anchor, not the
moving working tree. The committed T020 delivery is root `95e99e22f2`, docs
`2470275`. The [static audit](t004-comparison-readiness.json) confirms eight
affected production source files match the frozen common exactly and all eight
public fixtures match D001. It does not establish whole-tree equivalence:
concurrent Sketcher changes are excluded from these isolated comparisons.

Any added shared timing instrumentation creates a new common commit. Freeze its
exact source, generated outputs, build flags/binaries/dependencies, probes,
configuration, fixture and correctness receipts before acceptance samples.
Both arms receive the same instrumentation. Normal production builds keep it
disabled. Existing P07 binaries do not qualify newly constructed P01–P05/P08
arms. Final integrated-source applicability remains T010.

## Per-Change Construction And Ownership

| Change | Task | Construction and remaining qualification |
| --- | --- | --- |
| P01 buffered restore | T004.1 | Vary only scalar byte copy versus the current 64 KiB copy in `RestoreDocFile`. Both retain create/read/write/close diagnostics, notification boundaries, non-writable reuse, permissions and later-entry behavior. P02 save and P07 lazy loading stay identical. Rerun T002/T018 byte, hook, failure and document checks on both arms; freeze actual fault hits and outcomes. |
| P02 buffered save | T004.3 | Vary only the save copy algorithm. Its historical delta also adds a restore bad-read check: reversing the whole patch would remove safety. Keep current restore and all save failures on both arms. Time an isolated copy, never the original document. |
| P03 element-map parser | T004.3 | Compare allocation/tokenization algorithms with the same checked numeric grammar, range handling, aliases, child SID/hasher ownership and lookup semantics. Do not restore unsafe historical `strtol` behavior. Prove nonzero actual parser records for both primary fixtures; rerun T015/T016/T005 identity checks. Static XML/map-like entry counts are insufficient runtime coverage. |
| P04 direct 32-bit conversion | T004.2 | Reconstruct the bounded per-pixel baseline for the direct formats only, retaining the shared current fallback. Simply disabling the direct path would compare against today's Qt fallback rather than the historical per-pixel algorithm. Preserve extents, arithmetic, alpha/channel/orientation and ownership checks. |
| P05 Qt fallback conversion | T004.2 | Reconstruct the bounded historical pixel-color fallback, including its logical component/storage semantics where valid. Preserve the current direct path on both arms. Qualify each recipe at 512 and 4096, actual format/stride/pixels and original 1/2-component storage. The 18 T020 absolute storage rows do not qualify these generated recipes or a comparative P05 result. Failure to preserve fidelity safely requires an explicit reviewed exclusion, not an unsafe arm or a fabricated pass. |
| P06 defer unavailable payloads | T020 disposition, consumed by T004 | Preserve the reviewed exclusion in `t020-measurement-review.md`: independently non-identifiable under T006's shared restore/import guard. Zero premature loads remains required; no separate speedup claim. |
| P07 lazy full resolution | T004.2, retained candidate | Consume both immutable T020 campaigns and resource observations. In every non-P07 comparison retain identical lazy behavior. Diagnose/remedy first-use and control failures before any justified new P07 campaign; no automatic eager reversion. |
| P08 tree status coalescing | T004.3 | Vary scheduling/coalescing only if both arms retain T014 receiver lifetime and T017 success/error/direct-restore/import completion. Qualify T007 semantics and actual schedules. If those invariants make the historical algorithm non-identifiable, seek a reviewed explicit disposition before sampling. |

The machine audit records full historical origin pairs, delta hashes, fixture
inventories and owners for all eight changes. Those hashes describe lineage;
they are not qualified new-arm deltas. Record the latter separately after review.

## Bounded P07 Diagnostic Stage

This stage is exploratory and **ineligible for acceptance**. It addresses the
identified first-use cost and high variability; it does not rerun the study to
search for a pass. Preserve original samples, datasets, budgets and assessors.

1. Prepare shared diagnostic instrumentation separating image decode, conversion,
   native upload/render completion and event/scheduling wait. Record nested or
   overlapping intervals explicitly; do not add overlapping durations or infer
   physical GPU residency. Keep the qualified native correct-frame endpoint,
   source/pixel/calibration witnesses and pending/materialized/load counters.
2. Use isolated worktrees and the same locked RelWithDebInfo profile, native X11,
   Intel Mesa renderer, viewport/DPR/MSAA, preferences, camera and addon identity.
   Record current kernel/toolchain/driver, affinity, power policy, memory pressure,
   CPU/load and thermal observations where available. No global power changes,
   cache drops or termination of user applications are selected.
3. Run no concurrent agent build, test or native worker. Sample aggregate host
   CPU/load/available memory at one-second intervals for 30 seconds before and
   throughout the diagnostic window. Preserve the host trace without publishing
   unrelated process names/arguments. This is an observed desktop environment,
   not an asserted idle or dedicated machine. Unsupported required renderer or
   viewport stops execution. Competing load is a finding, not an exclusion rule.
4. Construct both diagnostic arms from the new shared instrumented common commit,
   retaining all safety/calibration fixes. A=eager at the payload-ready
   `finishRestoring` barrier; B=retained lazy. Only the reviewed P07 visibility
   condition may differ, exactly as in T020. Bind both exact commits and the
   complete allowed delta before any exploratory execution.
   Bound the entire diagnostic to 16 fresh GUI workers: one AB pair then one BA
   pair for each hidden, all-visible, PartDesign and VRML context. Retain each
   observation, including failures; no replacement, retry, optional extra pair
   or statistical acceptance assessment. Use the existing 60-second worker and
   256 KiB stream limits, with a 20-minute total wall-clock ceiling. Stop on an
   ownership, provenance, fidelity or safety failure; report remaining coverage
   as unexecuted. Publish no private fixture names, locations or images.
5. Stop after the fixed diagnostic and report attribution limits, host correlation
   and concrete findings. Choose one bounded correction only when supported by
   evidence. If none is justified, keep P07 unaccepted and present the unresolved
   measurement/latency issue; additional sampling needs a new reasoned plan.

These diagnostic counts and host observations do not amend D001's sampling
profile. If reliable acceptance requires a different host/isolation profile,
record the exact measured condition change and a dated decision before a new
freeze. Do not remove unfavorable samples from any completed campaign.

## Correction And Affected Revalidation

Do not choose speculative asynchronous loading, reduced-resolution placeholders
or calibration changes here. First identify whether decode, conversion, native
upload or scheduling dominates. Any proposed behavior change needs a scoped
design and owner decision when it changes product semantics.

A concrete source, probe or measurement-condition correction is required before
a new P07 acceptance campaign. Record its rationale, new decision ID, exact
common and arm commits, source deltas, configuration, worker/collector and build
receipts. Requalify T003 fidelity, T006 lifecycle/calibration/import/save-reopen,
T007 tree completion, trace ownership and native frame/context controls where
affected; use reviewed exact equivalence only for unaffected evidence. Then run
all seven fixed P07 cells once from pair zero under the unchanged D001 rules,
without combining campaigns. Retain fail/inconclusive classifications if repeated.

P01 independently requires a restore-payload interval probe (including failure
outcome checking), command-open completion, meaningful payload coverage and all
five declared fixture cells. T020 command auxiliaries are P07 descriptive data;
they cannot satisfy P01 or instrument its restore hook. Before P01 acceptance,
qualify both arms, validate the actual controlled source difference, check all
execution identities and freeze the exact schedule. No P01 sample is authorized
by static preparation alone.

## Execution Order And Stopping Points

Finish and review T022's plan; begin T004.1 construction/qualification, then
T004.2 image comparisons and P07 remediation, then T004.3 parser/tree/save.
Retain the task dependency order. Record preparation separately from completed
measurements. Do not claim a combined open-time improvement for an individual
optimization or close T004 using T020 memory savings alone.

The next concrete T004.1 work is a safety-preserving scalar-copy arm and common
hook instrumentation. Compile and run the affected correctness/failure matrix
before constructing the five-cell acceptance freeze. Any safety mismatch stops
that comparison and stays owned by T004.1.

## Source Preparation And Review

The [draft P01 construction](t004-source-review/p01-arm-construction.json) and
[review patch](t004-source-review/p01-scalar-baseline.patch) replace only the
restore copy loop. The draft is not built or runtime-qualified. Source review
found no blocker; byte, hook and document failure outcomes still need execution
on both arms. The patch applies cleanly to the common source; production source
is unchanged.

Independent reviewer `t004_plan_review` checked the actual plan, origin delta
hashes, common source/tree, fixture audit and draft patch. Two planning blockers
were corrected: include `ViewProviderImagePlane.h` in the eight-file source
audit, and define the diagnostic eager/lazy arms and allowed delta explicitly.
No review or static check is a new performance observation.

Final independent review found no remaining plan/source-review blocker after the
two corrections. D005 adopts this technical plan on 2026-11-08; acceptance remains
unfulfilled. No native worker ran during this planning/source-construction slice.

## D006 P01 Control Coverage Clarification

The public archive audit identifies one included payload in BIMExample (519 bytes)
and ArchDetail (5487 bytes), and none in EngineBlock or PartDesignExample. D006
preserves all five fixed cells while marking only the latter two controls'
restore_payload_ms as structurally not_applicable. Open/resource/identity checks
and all numeric rules remain required. Their native counters, bytes and duration
must all be zero; the other three cells require positive runtime payload coverage.
This decision is recorded before P01 sampling, following independent review.

## Execution Checkpoint After P01 Collection

T004.1's comparable measurement is complete. All five cells and both audits are
valid; P01 fails overall D001 because of BIM open p95 and inconclusive controls.
Current results and exact evidence are in t004-p01-results.md/json. This plan's
construction/future-tense wording records its pre-execution checkpoint. T004 owns
P01's acceptance residual; T004.2 image/P07 comparisons are next. No new budget or
production acceptance follows from completing the measurement task.
