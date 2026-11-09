---
title: Document performance evidence and scoped tradeoffs
doc_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-09
---

# Document Performance Evidence And Scoped Tradeoffs

## Purpose

Record the current local cost-benefit decisions for retained document/image
optimizations. These decisions accept particular workflow tradeoffs; they do not
constitute final production, upstream or cross-platform approval.

## Source Of Truth

Source and tests define implemented behavior. Spec003's immutable D001 results
record original statistical outcomes. The owner-selected D007 policy makes
numeric breaches review triggers while retaining correctness, fidelity,
calibration, safe arithmetic/lifetimes, error behavior and provenance hard gates.
The [T004.4 review](../specs/003-freecad-performance-optimization/t0044-cost-benefit-review.md)
and its [machine-readable bindings](../specs/003-freecad-performance-optimization/t0044-cost-benefit-review.json)
are the detailed decision record. T010 confirms final source/build/platform
applicability for the declared Linux scope; the whole spec remains draft pending T011.

## Assumptions

Evidence is local to the pinned source, fixtures, formats, dimensions, build and
runtime profiles in the linked receipts. Population workload frequency is unknown.
Do not infer subjective insignificance, host-load causality, GPU residency,
long-lived RSS reclamation or total-memory neutrality from these campaigns.
Scope limits below describe this review's approval, not software restrictions.

## Reference Data

| Path | Retained local decision | Evidence limitation |
| --- | --- | --- |
| P01 buffered restore | Payload-rich ordinary interactive/non-deadline opens: primary console-open median 1183 to 704 ms | BIM p95 2785 to 3348 ms accepted as scoped risk, cause unknown; no latency SLA |
| P02 buffered save | Payload-rich serial saves: paired full-save benefit 519 ms, CI 435–645 ms | p95 improves; maximum 5055 to 5193 ms; 64 KiB temporary/save, about 2 MiB higher primary process peak is not retained-memory proof |
| P03 bounded parser | Local map-hook gains, preserving qualified safe grammar and identity | Whole-open speedup unproven; Draft p95 804 to 1003 ms and maximum 1053 to 1618 ms; higher process peaks disclosed |
| P04 direct conversion | Large qualified local conversion gains despite dispersion | No universal GUI multiplier or concurrency claim |
| P05 fallback conversion | Accept memory-for-speed within the bounded inventory below | Long-lived/many-image RSS, replacement overlap and GPU memory unmeasured |
| P06 shared restore/import guard | Retain reviewed non-identifiability exclusion; zero premature loads mandatory | No independent speed claim |
| P07 lazy images | Hidden/subset workflow: hidden-open median 3321 to 1737 ms; hidden maximum RSS about 89 MiB lower | First-show median 858 ms, p95 1843 ms, maximum 1931 ms; no sub-second guarantee or immediate-all-images performance approval |
| P08 current tree path | Retain source-reviewed attribution exclusion with lifetime/fallback/finish correctness | Historical blanket suppression cannot be safely isolated; no separate current speed claim |

For P05, this local review selects at most three materialized 4096-equivalent
fallback planes, each dimension no larger than the qualified 4096 case, and one
active conversion with adequate actual memory headroom. This is a deliberate
bounded scope supported by logical accounting, not a tested three-fallback-image
campaign or a population estimate. Per 4096×4096 conversion, the extra decoded
temporary is 64 MiB. Extra retained Coin storage is 48 MiB per indexed-gray plane,
32 MiB per gray-alpha/Gray16 plane, zero for the tested premultiplied/SVG cases.
Thus three indexed-gray planes account for 144 MiB extra retained payload, or
96 MiB for the other two expanded formats, separately from the 64 MiB active
temporary. At 512×512, these payload terms scale by 1/64.

More generally, extra retained payload is 48N or 32N MiB for N materialized
4096-square planes of the respective formats, and extra temporary payload is
64K MiB for K simultaneous conversions. These are distinct logical inventories,
not an RSS ceiling or a complete RAM budget. Original decoded images, base Coin
storage, allocator behavior, old/new copies, model memory and driver/GPU storage
also require headroom. More than three planes, overlapping conversions/replacement,
larger dimensions and sustained allocator reuse require an extension review
before claiming the same performance approval. Functionality remains available.

P07 avoids materialization only for images never shown; otherwise it defers the
work. Repeat display of a materialized image adds no image load in the qualified
fixture. Once all images are visible, the measured RSS saving disappears and
all-visible p95 worsens. The separate n=2 diagnostic suggests read/decode dominates
first-use time; it is not a measured tail acceptance sample and is never pooled
with the 20-pair campaign.

## How To Update

Use the [review runbook](../runbooks/freecad-performance-optimization.md). Preserve
historical policies/results/receipts. For changed source or scope, assess affected
applicability and qualify actual gaps; do not repeat a complete campaign merely
to clear an old numeric trigger. Record a new dated decision rather than silently
broadening this scope. Owner: local-developer.

## Related Docs

- [Performance review runbook](../runbooks/freecad-performance-optimization.md)
- [Benchmark storage runbook](../runbooks/freecad-benchmark-storage.md)
- [Spec003 tasks](../specs/003-freecad-performance-optimization/tasks.md)

## Final Linux Qualification And Historical Numbers

[T010 final evidence](../specs/003-freecad-performance-optimization/t010-results.md)
identifies the integrated base plus corrective patch and declared Linux scope.
Its source guard includes App-only restore before GUI restore; preview hiding
is idempotent. Two new ready/late regressions and final native load/close traces
cover these states. The original benchmark classifier omitted App-only restore;
old zero counts are not complete lifecycle proof.

Numbers in the earlier table are historical campaign observations. P01/P07
external embedded-Python DSO closure was not fully bound; their old latency,
tail, whole-open and RSS magnitudes are not exact-final/current guarantees.
P02/P03 used isolated workers and full runtime maps. Current acceptance retains
scoped local implementation directions and hidden/subset first-use/logical-RAM
tradeoffs without a numeric response SLA. Current absolute/tail production
claims require new predeclared measurements under the selected runtime.

The final native viewer/memory profile is Linux X11 softwareGL with compose and
Qt generic/Fusion. Process-level user-site isolation and actual module hashes
are part of the evidence. Windows/macOS/Wayland/other drivers and GNOME/IBus memory
qualification are excluded. This is local integrated qualification, not upstream
or release approval. T011 owns final spec closure.
