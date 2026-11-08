---
title: T020 measurement and optimization disposition
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-08
---

# T020 Measurement And Optimization Disposition

**T020 measurement/disposition is complete. P07 is NOT ACCEPTED under D001.**
Phase 3 exit, T004 and T010 remain blocked by mandatory T022/D005. No shipping,
speedup, behavior revision, accepted tradeoff or threshold waiver is implied.

Owner direction, 2026-11-08: **retain P07 as a candidate**, valuing its apparent
opening-time and observed RAM benefits. T022/D005 now focuses on reliable benefit
validation and appropriate first-use/tail criteria. The original 500/1000 ms
budget results below remain unchanged; retaining the candidate does not invent
a new numeric budget or approve final production acceptance.

| Campaign | Measurement validity | Optimization outcome |
| --- | --- | --- |
| [Freeze 1](t020-evidence/freeze-1-performance-results.md) | Six complete cells; hidden primary aborted and ineligible | Incomplete; original failed/inconclusive findings retained |
| [Corrected recovery 2](performance-recovery-2-results.md) | Seven valid complete cells; 336 observations; one attempt each | Hidden first-show and two non-regression gates fail; other timing inconclusive; resource gates pass |

[Machine-readable disposition](performance-results.json) binds both independent
records. No observation is pooled, resumed or selected between campaigns. The
first result/receipt bytes were preserved before this aggregate was written.

The corrected campaign measures lazy first-show median **857.91 ms**, exceeding
**500 ms**, and p95 **1843.09 ms**, exceeding **1000 ms**. Failed control timing
limits and high variability also remain visible; they do not establish a causal
P07 regression. Nominally lower open medians and lower image storage cannot offset
the failed first-show budget or establish an accepted speedup.

The hidden scenario opens with three pending textures, materializes only the
selected image on first show, and reuses it on repeat display. Two unshown loads
are avoided within that measured window; work needed on first display is deferred.
Calibration/full resolution and zero-premature-load correctness remain qualified.
GPU residency is not measured by the logical decoded/Coin counters or process RSS.

All exact raw observations, statistics, storage scope, source/build/worker/context,
qualification/controller/audit hashes and local Linux applicability are in
[the corrected-campaign report](performance-recovery-2-results.md). Eighteen
standalone direct/fallback format rows per original arm remain explicitly reused
by byte-identical native/source equivalence; P05/T004.2 comparisons are separate.

T022 must define the retained candidate's measurement/revalidation plan before
T004. Validate opening/RAM benefits, first-use and tail latency, repeat behavior
and responsiveness. Any agreed revised numeric criterion needs a dated protocol
decision and applicable new evidence. D005 remains open for that plan; dropping
lazy loading is not the selected direction. See
[the decision ledger](open-decisions.md).
