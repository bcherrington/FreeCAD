---
title: FreeCAD performance optimization technical design
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Technical Design

## Overview

Reconcile implemented document-open optimizations with the remaining correctness, measurement and
review gates. Keep compact rail geometry in package 002.

## Requirement Coverage

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
| --- | --- | --- | --- |
| Requirement 1 | AC1, AC2, AC3 | Included-file restore compatibility; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 2 | AC1, AC2, AC3 | Image conversion fidelity; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 3 | AC1, AC2, AC3, AC4, AC5, AC6 | Comparable optimization evidence; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 4 | AC1, AC2, AC3, AC4 | Element-map restore compatibility; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 5 | AC1, AC2, AC3, AC4 | Image-plane restore lifecycle; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 6 | AC1, AC2, AC3, AC4 | Tree restore status batching; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |

## Correctness Property Coverage

| Property | Design Behavior | Validation Direction | Notes |
| --- | --- | --- | --- |
| CP-001 | Buffered restore preserves bytes, entry boundaries, permissions and notifications with defined document-level failure outcomes. | T018, T002; archive-level failure and compatibility fixtures | Source correction/qualification pending |
| CP-002 | Image conversion and texture-only materialization preserve pixel semantics, original source, independently calibrated dimensions and placement. | T013, T003, T006; calibrated reopen/import/show/save fixtures | T013/T003/T006 local qualification complete; final T010 applicability remains |
| CP-003 | Tree batching preserves per-object effects, ends callbacks with receiver lifetime and completes every suppressed restore path. | T014, T017, T007; destruction and idle restore-path regressions | T014 lifetime and T017 completion locally qualified; T007 broader semantics pending |
| CP-004 | Image extents, allocation lengths and offsets are checked before narrowing or writing; unsupported input is rejected safely. | T012, T003; null/slender boundary tests and sanitizer-equivalent checks | Source correction/qualification pending |
| CP-005 | Numeric conversion is bounded and element-map aliases, child IDs and dependent CAD reference identities survive restore/recompute. | T015, T016, T005; numeric/writer/identity fixture matrix | Source correction/qualification pending |

## High-Level Design

### System Architecture

Keep archive I/O in App/Base and display work in Gui. Implemented source paths
are buffered property-file copy, element-map token parsing, direct bitmap
conversion, deferred full-resolution image-plane loading and pending tree status
updates. This package reconciles those paths and their remaining acceptance.
The MoE review found source/spec conflicts in image extent safety, calibrated
plane sizing and tree receiver lifetime. Presence of a fast path is not verified
acceptance of its safety or compatibility. T012–T018 own the source corrections
and regression cases; their validation evidence remains pending.
Compact rail geometry belongs to package 002; Sketcher diagnosis history remains
in archived package 000.

### Components and Changes

`src/App/PropertyFile.cpp`: bounded binary copy and read/write/close checks.
`src/Base/Reader.cpp`: registered archive-entry restore dispatch.
`src/App/ElementMap.cpp`: string_view dot tokenizer and numeric token conversion
instead of temporary split vectors; bounded numeric compatibility remains
T015/T016. `src/Gui/BitmapFactory.cpp`: direct 32-bit conversion plus Qt fallback.
`src/Gui/ViewProviderImagePlane.cpp`: pending full-resolution texture lifecycle.
`src/Gui/Tree.cpp`/`Tree.h`: pending restore status scheduling and receiver-owned
application subscriptions. T014 qualifies widget teardown. T017 keeps the owned
status timer scheduled during suppression so direct restore/import can resume
after guards exit, while retaining immediate application finish-open flushing.
Empty restored documents also receive an observer-attachment completion pass.

### Data Models And Flow

Archive entry → Base::Reader → PropertyFileIncluded::RestoreDocFile → temporary
file → read-only permissions and property notification. Image properties defer
while restoring; visible planes load on finishRestoring and hidden planes on
show in the inspected implementation. Required behavior is to keep original
source bytes and independently calibrated plane dimensions unchanged. The
inspected setPlaneSize path conflicts with that requirement after the restoring
flag clears; T013 must separate texture materialization from calibration updates.
No document format, cache schema or public API migration is intended.

## Low-Level Design

### Algorithms And Interfaces

Property restore uses bounded read/gcount/write loops, preserves the existing
non-writable-file early return and brackets changes with aboutToSetValue and
hasSetValue. Base::XMLReader::readFiles dispatches registered ZIP entries.
ElementMap scans dot-separated tokens without allocating a vector per name.
Bitmap conversion handles RGB32, ARGB32, RGBA8888 and RGBX8888 directly, flips
vertically, produces RGBA and finishes SoSFImage editing. Other formats,
including premultiplied alpha, use Qt conversion to RGBA8888 plus a vertical
flip. Do not claim a direct premultiplied scanline path.

ImageFile/XSize/YSize restore updates mark pendingFullResTexture; finishRestoring
loads visible planes and show loads pending hidden planes. Reduced-resolution
placeholders are not implemented. Tree restore batches status scheduling only;
_slotDeleteObject still preserves per-object focus, selection, children and map
side effects. Any bulk-delete bypass needs a separate measured proposal.

### Image Qualification Reference

T003 compares canonical RGBA8 against an independent QColor/pixelColor scalar
reference derived from the historical conversion approach. The old raw
sizeInBytes-based component inference is not an oracle for padded, alpha-only
or newer floating-point formats. Preserve source color/alpha semantics rather
than reproducing its unsupported layouts. Direct byte fixtures remain exact;
normalized premultiplied/high-depth scalar comparison permits at most one RGBA8
level for quantization. Real production SoQtOffscreenRenderer image-plane frames
must match the scalar luminance/LA/RGB/RGBA reference within two RGB levels per
pixel, with source-over color at texel centers within three levels. Orientation,
transparent-background blending and world-space scale/placement have independent
analytic assertions; bounding rectangles are exact with zero MSAA. These rules
are set before executing the render matrix. Qualify the native X11/xcb framebuffer
profile required by T019 and record supplementary Qt offscreen results separately.
Framebuffer correctness is distinct from T020's interactive native-view
first-correct-frame timing and T010's platform coverage.

### Error Handling And Compatibility

Preserve binary bytes, entry boundaries, destination permissions and observable
filesystem failures. Verify image format fallback, alpha and texture ownership
rather than extrapolating from four conversion tests. Reopen saved documents,
exercise hidden/visible image transitions and preserve tree/observer behavior.
Do not add production tracing or broader caching without separately reviewed
invalidation and diagnostic contracts.

### MoE Safety And Compatibility Contracts

- **Image extents (F001):** before narrowing dimensions into Coin types, validate
  non-null image, positive representable extents and checked destination/offset
  sizes. T012's QImage-to-Coin conversion accepts positive extents up to
  `std::numeric_limits<short>::max()` (32767 here), with RGBA bytes bounded by
  `qsizetype::max()` using division before multiplication. It clears the output
  to `(0,0)`, zero components and no pixels for null/unsupported input or failed
  fallback format conversion. Direct row offsets use `std::size_t`; narrowing
  follows validation and Coin's four-byte allocation alignment cannot overflow.
  The void API is preserved. Allocation exceptions from Coin remain exceptions;
  this is not an out-of-memory recovery policy. D001/D003 identify backend/build;
  T012 covers both orientations of 32767/32768/65537 slender images and valid
  edge pixels; broader fidelity/calibration qualification remains T003/T006.
- **Calibration (F002):** T013 uses an explicit private image-size policy:
  direct new/replaced image assignment initializes DPI-derived physical sizes;
  pending materialization preserves stored XSize/YSize after transient flags
  clear, while still refreshing image DPI metadata and original texture data.
  Both GUI restoring and App ObjImporting defer ImageFile/XSize/YSize loads.
  Placement and calibrated Coin quad coordinates remain unchanged. The existing
  Gui::Document completion ordering is retained; the provider owns this policy.
  Real offscreen App/Gui archive/copy/import regressions qualify the local fix;
  T003 qualifies local image/render fidelity; T006 qualifies the expanded lifecycle.
  Ordinary show defers while GUI restoring or App ObjImporting without consuming
  the pending request. Object finishRestoring is the explicit payload-ready barrier:
  App import extracts payloads before afterRestore emits completion, although
  ObjImporting remains set until import returns. Visible pending planes load there;
  hidden planes retain the request until first show. No Document ordering patch
  is required. Failed/empty/unsupported conversion clears the old texture and
  preserves calibration/geometry; physical DPI initialization follows successful
  conversion only. A failed nonempty load warns and consumes the request; an
  explicit valid replacement recovers using insertion sizing. Repeated show of a
  materialized plane retains its cache even if the backing file disappears. Empty
  property Paste/undo clears the texture; existing blank-string setValue semantics
  remain unchanged. Included payloads survive deletion of the external original.
  T018 owns broken-archive save/metadata outcomes, and T010 owns final environment
  qualification, including the unsuccessful Qt/IBus lifecycle Memcheck attempts.
- **Receiver lifetime (F003):** start/finish-open subscriptions and scheduled
  callbacks must not outlive TreeWidget. T014 explicitly disconnects both App
  open subscriptions in the destructor alongside the seven existing Gui
  subscriptions, before removing the receiver from the instance registry. The
  unscoped connection member type is unchanged. Owned QTimers and receiver-context
  queued calls cancel with QObject lifetime; the existing delayed focus callback
  also supplies its receiver context. Direct and parent destruction, real App
  document open after deletion, repeated replacement, and queued status work pass
  under Memcheck. This preserves the existing GUI-thread dispatch assumption;
  no concurrent signal/teardown contract or broader scheduler change is added.
  Tree.h is inspected unchanged. T017 qualifies local completion; T007 still owns
  the larger-model/object-effects checkpoint.
  The new internal-widget test is registered outside Windows because TreeWidget
  lacks DLL export; no public export is introduced for testing. T010 owns final
  platform/native qualification.
- **Numeric and CAD identity (F004):** tokenization and numeric conversion are
  both implementation changes. T015 defines bounded numeric conversion and
  legacy tolerance for signs, whitespace, prefixes, invalid suffixes and range
  limits. T016 covers semicolon/colon/dollar-hashed forms, decimal child SIDs
  versus hexadecimal mapped SIDs, multiple aliases/SIDs and nested/shared maps.
  Compare semantic save/restore/save and bidirectional name lookup after input
  buffers die; a dependent-feature CAD fixture must retain referenced targets
  after reopening/recomputing. CP-005 records these identity invariants.
- **Restore completion (F005):** suppression must have a matching completion for
  app open, direct doc.load/doc.restore, import/paste and failure cleanup. T017
  retains broad isRestoring guards against processing and keeps one existing
  owned status QTimer scheduled with one pending flag. Immediate requests become
  delayed during restoration; retry interval is at least 1ms while restoring,
  including a configured zero timeout. Application finish-open requests a flush
  without clearing the flag early. Other restore/import exits resume through the
  timer after the global guard unwinds. The flag clears only when processing
  proceeds past restore/transaction guards, and successful processing stops the
  timer. New restoring document items schedule a pass even without queued objects
  to attach deferred change/touch observers. No App/Gui document signal, flag,
  archive policy, public API, member layout or additional subscription changes
  are needed. T017 qualifies small real-document idle-timer/error paths; T007
  retains larger-model, focus/selection and per-object effects acceptance.
- **Archive errors (F006):** preserve established compatibility or document an
  approved deliberate change. T018 records empty/nonempty read/write/close
  failures at document level, diagnostics/failure lists, partial-file handling,
  notification state and later-entry continuation. XMLReader catches hook
  exceptions, so hook-only assertions cannot prove archive-level acceptance.

### Evidence Applicability And Benchmark Contract

T019 selects [validation-manifest.md](validation-manifest.md) and its immutable
v1 JSON intake snapshot (`spec003-linux-x86_64-debug-candidate-20261106-v1`) before dependent acceptance work:
source/dependency commits, binary provenance, platform/toolchain/build flags,
fixture hashes and the allowed workflow. D003 resolves this initial selection;
T010.1 confirms final production revisions and invalidates affected results.
Source/build/dependency/fixture/environment changes require reruns of affected
correctness, archive, calibration, lifecycle, manual/visual and performance checks
or explicit reviewed equivalence. A build or a final timing assembly does not
by itself revalidate evidence from a different composition.

D001/T001 selects [benchmark-protocol.md](benchmark-protocol.md) and its linked
JSON/collector. They predeclare primary metrics/fixtures, sample count/statistic,
warm-up/exclusion/cache policy, variability handling, and improvement and
non-regression pass rules before samples are collected. Inventory buffered
restore, any retained buffered-save patch, element-map parsing, image conversion,
lazy loading and tree batching with per-change comparative evidence ownership.
Combined benchmarks are permitted only with a stated attribution limit; each
retained change needs assigned evidence or an explicit reviewed exclusion/route.

T020 and T004 separate command-mode restore from GUI open-to-first-correct-frame,
hidden-first-show-to-correct-frame and repeat-show. Define completion signals,
initial visibility and pending/materialized texture counts. Record peak process,
decoded and Coin image storage for representative direct and 1/2-component
fallback sources; qualify GPU residency only with driver-specific measurements.
Report work shifted to first display separately from work eliminated, and apply
D001's approved latency and material memory-regression rules.

## Validation Strategy

The 2026-11-04 focused results remain imported evidence on the recorded source
revision. T019 pins candidate validation provenance and T001 approves the benchmark contract; T002/T003/T005/
T006/T007 cover restore, image, parser, lifecycle and tree correctness after
T012–T018 correction gates. T020 records deferred-display latency and memory;
T004 owns comparative restore/image/parser/tree evidence. T010 reconciles exact
final revisions, reruns affected acceptance evidence, builds and focused/full
checks before reviewer evidence is assembled. Historical timings in
legacy-tasks.md/research.md are ranking signals, not current speed guarantees.
Conditional GUI/startup, reduced-resolution and additional tree optimization
investigations have owned backlog routes; do not reopen working code solely
because historical checklists drifted. Current MoE defects remain required fixes.

## Execution Phases And Checkpoints

The authoritative execution order is [tasks.md#execution-order](tasks.md#execution-order),
not numeric task sorting or the first dependency-ready item offered by a tool.
`Depends on` encodes the chosen serial delivery order as well as technical edges.
Source implementation facts and dated results remain in
[baseline-status.md](baseline-status.md); they do not set executable task status.

1. **Planning:** retain T021/T019 evidence, record T009's B009 scope disposition,
   and retain T001's completed runnable D001 protocol before any new source work.
2. **Correction/qualification:** finish image safety/calibration/fidelity,
   tree lifetime/completion, parser/CAD identity and archive compatibility in
   that order. Each fix includes its own regression; each subgroup ends with
   current qualification evidence before the next subgroup begins.
3. **Performance:** execute T020 then T004 against the predeclared contract,
   recording deferred work, memory, per-change attribution and non-regression.
4. **Delivery:** T010 confirms exact final applicability and broad/platform
   checks; T011 promotes durable truth and audits owned residuals before closure.

Conditional new investigation is owned outside the critical path: B009 reduced
resolution, B010 GUI restore/startup profiling and B011 additional tree
optimization profiling. Current source-safety fixes, functional checks and
comparative evidence for retained tree/parser/image/restore changes are never
waived by that routing. A routed optional child is not a runtime pass, and parent
acceptance explicitly excludes its future execution while requiring owned routing.

New comparative results belong in performance-results.md/performance-results.json
(created by Phase 3), with the T001 contract in benchmark-protocol.md. Historical
research.md and legacy measurement/checklist files remain immutable inputs.

## Downstream Task Guidance

Resolve the decisions in open-decisions.md blocking the selected task, read its requirement criteria
and obtain task context before coding. Preserve existing source changes. Run
focused checks for code changes and broaden according to root AGENTS.md.
Reconcile design/tasks/traceability after changing a decision. No new testing
dependency is proposed; use existing fixtures, conventional tests and bounded
manual checks.

## Operational Considerations

Promote accepted current behavior to `docs/reference/document-open-performance.md` and operational
validation
to the runbook target in requirements.md before closure. These are proposed
destinations, not claims that those documents already exist. Route residual work
to docs/backlog/README.md with local-developer ownership. Keep the active package
until acceptance, reviews, promotion and closure metadata are complete.

## Open Questions

The authoritative unresolved decision ledger is [open-decisions.md](open-decisions.md).
D001 is resolved with the predeclared measurement/pass-rule protocol; Phase 1
is complete and T012 starts Phase 2. Resolved D002 routes
optional reduced-resolution exploration to B009 while keeping full resolution;
resolved D003 pins the initial local candidate and applicability policy.
T010.1 still confirms the exact final production composition and affected evidence. T021 verifies discovery of all three IDs and distinguishes
advisory context availability from execution/acceptance readiness. The ledger,
task dependencies and literal acceptance criteria must agree before selection.

## Related Artifacts

- [Requirements](requirements.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Open decisions](open-decisions.md)
- [Candidate validation manifest](validation-manifest.md)
- [Baseline implementation inventory](baseline-status.md)
- [Optional investigations](../../backlog/spec003-follow-ups.md)
- [MoE findings](verification.md#moe-review-findings)
- [Migration input](legacy-spec.md)
