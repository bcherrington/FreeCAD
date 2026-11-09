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
are historical decision evidence, not the current operating contract. T010 confirms final source/build/platform
applicability for the declared Linux scope. T011 promotes this contract; the
package is retained as history after local closure. Source implementation commit:
`21903ca31f`; T010 evidence/docs snapshot: `756c03a`.

## Assumptions

Evidence is local to the pinned source, fixtures, formats, dimensions, build and
runtime profiles in the linked receipts. Population workload frequency is unknown.
Do not infer subjective insignificance, host-load causality, GPU residency,
long-lived RSS reclamation or total-memory neutrality from these campaigns.
Scope limits below describe this review's approval, not software restrictions.

## Implemented Compatibility And Lifecycle Contract

Source and native tests establish implemented behavior and must agree with this
accepted contract. A mismatch blocks affected acceptance until reconciled through
compatibility tests and review; a regression does not silently redefine the contract.

| Area | Contract | Source and regression evidence |
| --- | --- | --- |
| Included-file restore/save | Preserve binary bytes, empty/multi-buffer and multi-entry boundaries, property notifications, read-only output and observable errors. Copying remains bounded; archive format and public API are unchanged. Existing read-only destinations retain their bytes. Embedded file/data names must be plain basenames: reject absolute paths, directory components and dot/dot-dot references before resolving beneath the document transient directory. | `src/App/PropertyFile.cpp`; `src/Base/Reader.cpp`; native included-file/Reader tests |
| Payload failure | Reader catches hook failures, continues later entries and does not set document RestoreError/PartialRestore for these failures. Nonempty failures emit errors and enter FailedFiles; empty failures emit a skipped-empty log and do not enter that list. Partial destinations and property paths are retained; unsuccessful writes are not successful payload completion. | Native document-level read/write/close/create failure injection plus later-entry byte checks; historical T018/T002 receipts |
| Conversion | Validate non-null positive dimensions, Coin representability and allocation/offset arithmetic before narrowing or allocation. Preserve source color/alpha/orientation and ownership across direct and fallback formats. Null/unsupported extents fail without partial pixel writes. | `src/Gui/BitmapFactory.cpp`; bitmap, extent and rendered image tests |
| Image restore and first show | Defer original full-resolution texture loading throughout App object restore, GUI restore and import. Visible completion or first show materializes pending textures without changing independently calibrated XSize/YSize or placement. Failed pending loads warn and consume the pending request safely, clearing stale texture data. Repeat show reuses materialized data. Fresh insertion/explicit resize retain existing behavior. | `src/Gui/ViewProviderImagePlane.cpp`; ready/late App-only restore, GUI/import, calibration and native-viewer checks |
| Element maps | Decimal child SIDs and hexadecimal mapped fields retain bounded legacy conversion: whitespace, one sign, optional hex prefix, longest valid prefix stopping at the first invalid byte (including embedded NUL), zero for no digits and saturation at long limits. Validate mapped indices in 0..INT_MAX and metadata counts/intervals before allocation/narrowing. Own compact prefix storage before temporary tables expire. Missing child SIDs warn/omit safely; alias/child-map and bidirectional reference identity remain intact. | `src/App/ElementMapRestore.h`; `src/App/ElementMap.cpp`; numeric, naming, alias/child-map and dependent-CAD roundtrip tests |
| Tree | Coalesce restore status work, then flush/attach observers at completion or safely resume fallback scheduling after direct load, import and failure. End application subscriptions and queued work with receiver lifetime. Preserve per-object selection, focus, hierarchy/maps and observer effects on deletion. | `src/Gui/Tree.cpp`; lifetime, idle completion, failure, selection and pinned-model tests |
| Preview teardown | Hiding a preview removes its scene child only if still attached; repeated hide/close is safe. | `src/Mod/Part/Gui/ViewProviderPreviewExtension.cpp`; final native zero-warning close trace |

Failure notification ordering is observable: successful payload completion sends
one before/after pair; failed payload-phase work after assignment sends before
without after; create/open failure and existing read-only output send neither.
XML property assignment has separate notifications. A rejected nonexistent-file
assignment retains prior payload/path/basename but updates the original-file-name
metadata before validation. Saving after included-payload removal throws a
filesystem error, preserves the prior saved archive and leaves a UUID-suffixed
temporary archive; no new rollback or automatic cleanup policy is introduced. An empty payload performs no
write, so a write-fault hook cannot qualify an empty write failure. Exact retained
bytes after injected failures are test observations, not storage guarantees.

The image oracle uses independent scalar QColor semantics, exact direct-byte
fixtures and at most one RGBA8 level for normalized premultiplied/high-depth
quantization. Qualified rendered luminance/LA/RGB/RGBA differences allow two RGB
levels per pixel and three for source-over texel centers, with independent
orientation/placement/scale and exact zero-MSAA bounds. These tolerances are not
permission to reduce source resolution. Model identity checks compare persistent
references, topology and geometric invariants; byte identity is required only for
deterministic writer fields. The separate Assembly sphere-basis normalization
supplement is tightly scoped in historical evidence and does not alter original
campaign oracles.

## Cost-Benefit Decision Contract

D007 makes numeric limits investigation triggers. It never waives correctness,
fidelity/calibration/model-reference identity, safe arithmetic/allocation/lifetime,
archive compatibility/error visibility, source/build/runtime/fixture provenance,
or declared applicable coverage and bounded collection.

Every retained optimization or attribution exclusion needs an owner, date and:

1. Original outcome and investigation trigger.
2. Evidence and exact source scope.
3. Absolute and relative benefit.
4. Paired confidence and tail behavior.
5. Operation frequency, explicitly unknown where unavailable.
6. Transient and retained memory plus process peaks.
7. Concurrency and selected operating scope.
8. Uncertainty and applicability.
9. Rationale and mitigation.
10. Owner and residual tasks.
11. Decision and acceptance scope.

Current selection is retrospective: preserve original D001 classifications,
policies, samples, aborted attempts and receipts. Future samples freeze policy,
review fields and comparison inputs first. A policy-only change does not require
a repeat campaign solely to remove an old numerical veto. A source, dependency,
build, fixture or relevant runtime change requires affected requalification or
independently reviewed equivalence tied to the actual final composition.
Dispersion is interpreted with absolute effect, confidence, tails and host context;
host-load causality is not inferred without measurements. No universal RAM or
response-time allowance is created. Keep archive/parser hooks, conversion time,
console open, first correct GUI frame, hidden first show and repeat show distinct.
Report eliminated work separately from deferred work and memory never allocated.

## Reference Data

| Path | Retained local decision | Evidence limitation |
| --- | --- | --- |
| P01 buffered restore (historical numbers; external Python closure incomplete) | Payload-rich non-deadline opens: historical console-open median 1183 to 704 ms | BIM p95 2785 to 3348 ms accepted as scoped risk, cause unknown; no latency SLA |
| P02 buffered save | Payload-rich serial saves: paired full-save benefit 519 ms, CI 435–645 ms | p95 improves; maximum 5055 to 5193 ms; 64 KiB temporary/save, about 2 MiB higher primary process peak is not retained-memory proof |
| P03 bounded parser | Local map-hook gains, preserving qualified safe grammar and identity | Whole-open speedup unproven; Draft p95 804 to 1003 ms and maximum 1053 to 1618 ms; higher process peaks disclosed |
| P04 direct conversion | Large qualified local conversion gains despite dispersion | No universal GUI multiplier or concurrency claim |
| P05 fallback conversion | Accept memory-for-speed within the bounded inventory below | Long-lived/many-image RSS, replacement overlap and GPU memory unmeasured |
| P06 shared restore/import guard | Retain reviewed non-identifiability exclusion; zero premature loads mandatory | No independent speed claim |
| P07 lazy images (historical numbers; external Python closure incomplete) | Hidden/subset workflow: hidden-open median 3321 to 1737 ms; hidden maximum RSS about 89 MiB lower | First-show median 858 ms, p95 1843 ms, maximum 1931 ms; no sub-second guarantee or immediate-all-images performance approval |
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
or release approval. See the [closure record](../history/spec-closure-log.md)
for package disposition and outstanding delivery limits.
