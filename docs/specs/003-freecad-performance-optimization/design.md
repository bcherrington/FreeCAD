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
| CP-002 | Image conversion and texture-only materialization preserve pixel semantics, original source, independently calibrated dimensions and placement. | T013, T003, T006; calibrated reopen/import/show/save fixtures | Source correction/qualification pending |
| CP-003 | Tree batching preserves per-object effects, ends callbacks with receiver lifetime and completes every suppressed restore path. | T014, T017, T007; destruction and idle restore-path regressions | Source correction/qualification pending |
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
application subscriptions. T014/T017 must reconcile widget teardown and all
suppressed restore completion paths; application finish-open alone is not a
complete contract.

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
  sizes. D001/D003 records include the supported backend/build. T012 defines a
  documented failure result for unsupported extents without adding a public API
  casually; cover 32767/32768/65537 slender boundaries and valid edge pixels.
- **Calibration (F002):** T013 requires texture-only load of restored/imported
  image planes. Existing XSize/YSize and placement survive finishRestoring,
  hidden first show and save/reopen independently of GUI restoring/ObjImporting
  flags. Preserve intentional initial-image sizing and explicit user resize.
- **Receiver lifetime (F003):** start/finish-open subscriptions and scheduled
  callbacks must not outlive TreeWidget. T014 may use explicit disconnection or
  established scoped connections; test actual destruction/recreation, not dock
  hiding alone. Include Tree.h and receiver ownership in scope.
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
  may narrow suppression to the owned operation or safely schedule broader
  completion; choose from inspected lifecycle evidence. Idle timers and no later
  edit/open must still yield eventual tree population/status/observer updates.
- **Archive errors (F006):** preserve established compatibility or document an
  approved deliberate change. T018 records empty/nonempty read/write/close
  failures at document level, diagnostics/failure lists, partial-file handling,
  notification state and later-entry continuation. XMLReader catches hook
  exceptions, so hook-only assertions cannot prove archive-level acceptance.

### Evidence Applicability And Benchmark Contract

T019 pins a candidate validation manifest before dependent acceptance work:
source/dependency commits, binary provenance, platform/toolchain/build flags,
fixture hashes and the allowed workflow. This is D003's initial selection;
T010.1 confirms final production revisions and invalidates affected results.
Source/build/dependency/fixture/environment changes require reruns of affected
correctness, archive, calibration, lifecycle, manual/visual and performance checks
or explicit reviewed equivalence. A build or a final timing assembly does not
by itself revalidate evidence from a different composition.

D001/T001 must predeclare primary metrics/fixtures, sample count/statistic,
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
Conditional GUI profiling, startup work and lower-resolution behavior are
separate decisions; do not reopen implemented code because old checklists drifted.

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
D001 approves the measurement/pass-rule contract, D002 dispositions the separate
reduced-resolution proposal, and D003 pins candidate/final revision and platform
applicability. T021 verifies discovery of all three IDs and distinguishes
advisory context availability from execution/acceptance readiness. The ledger,
task dependencies and literal acceptance criteria must agree before selection.

## Related Artifacts

- [Requirements](requirements.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Open decisions](open-decisions.md)
- [MoE findings](verification.md#moe-review-findings)
- [Migration input](legacy-spec.md)
