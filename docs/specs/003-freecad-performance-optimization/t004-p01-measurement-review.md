---
title: P01 measurement construction and qualification review
doc_type: spec
artifact_type: review
status: approved-for-planning
owner: local-developer
last_reviewed: 2026-11-08
---

# P01 Measurement Construction And Qualification

This is a pre-sample technical clarification under D001, recorded by Codex under
“continue” T004.1. Independent agent review is not human/maintainer approval.
No P01 acceptance sample has been collected at this checkpoint. Final concrete
freeze/controller review and the before-collection audit remain mandatory.

## Shared Safety And Payload Endpoint

Both arms retain all Phase 2 safety, notification, permission, archive failure,
calibration and CAD identity behavior. Only the restore copy algorithm differs:
scalar get/put versus the current 64 KiB read/write loop. P02 save, P07 lazy
full-resolution loading and all other optimizations remain identical.

The opt-in native probe records hook entry through copy/read/write/close checks.
Successful timing stops before permissions and notifications. RAII records
failures and reuse separately; there is no per-byte instrumentation. Duplicate
start/stop and active-scope reset are rejected. All 31 included-file/probe cases
and 26 Reader cases pass on each arm with no skips. The scalar arm exposed a
fault-injector gap: the old stream implemented only xsgetn. Shared underflow/uflow
support and an explicit scalar/block boundary regression repair the harness;
existing archive outcome assertions remain unchanged. Failed qualification
receipts remain ineligible historical diagnostics.

## Complete Console Open Endpoint

BIMExample and ArchDetail require completion work after openDocument returns.
D001 already ends document_open_ms at completed restore/recompute. The worker
checks non-null/not-restoring/not-recomputing, calls doc.recompute once only when
mustExecute is true, requires a positive integer return when invoked, then
requires all three completion flags false. No loop, force, retry or GUI setup.
The call stays inside document_open_ms; payload attribution remains unchanged.

Qualification freezes completion counts per fixture: primary 0, BIMExample 3,
ArchDetail 2, EngineBlock 0, PartDesignExample 0. Both arms agree. Samples must
match exactly. All objects, proxies, references, archive bytes/permissions and
normative hashes qualify after completion. GUI-only SVG warnings remain in the
private worker logs; they are not suppressed or treated as completed pending work.

## Required Runtime Assembly And Shared Parser

Normal locked CMake targets populate BIM, Draft, TechDraw/TechDraw_Data,
lazy_loader and ArchSqlParser. Earlier missing-type/import failures describe an
incomplete isolated runtime, not malformed fixtures or a production crash.

Independent SQL parser generations differ even with PYTHONHASHSEED=0. Both
original variants are retained. Select the change-arm generated artifact by the
fixed pre-sample construction rule and reuse its exact bytes in both arms with
independent output inodes. This is normalized shared runtime assembly, not a
claim of independently reproducible generation. Bind grammar, generator source,
Python/Lark identity, chosen bytes and normalization receipt. No Ninja run may
follow normalization without renormalization and regenerated qualification.

The worker verifies loaded generated_sql_parser.__file__ is the qualified build
path and its hash matches the shared artifact. Both BIM controls require it;
other fixtures freeze its absence. Fresh qualification passes all five fixtures
in both arms on this normalized runtime.

The only permitted output differences are lib/libFreeCADApp.so, the
PropertyFile.cpp object and App_tests_run. All other runtime/generated/resource
bytes must match. Both physical output trees have independent inodes.

## D006 Coverage And Collection Boundary

The private primary qualifies three included payloads totaling 57,246,043 bytes;
BIMExample qualifies one 519-byte payload and ArchDetail one 5,487-byte payload.
EngineBlock and PartDesignExample have none. D006 marks only their payload metric
not_applicable, never a payload pass. Their zero counters/bytes/duration and all
open/resource/model/reference rules remain required. Keep all five fixed cells.

Freeze Root Pixi manifest/lock, current task gates, live installed-package
inventory, exact source/dependency and native/runtime inputs, worker/collector/
assessor sources, all underlying build receipts and qualified contexts. Audit
before and after collection. Each cell gets exactly one 24-pair attempt: one
diagnostic pair, three discarded warmups and twenty measured pairs in D001 order.
A worker/provenance failure stops further cells; no retry, pooling, outlier
removal, extra pair or threshold revision. Statistical outcomes are assessed only
after complete collection and the final audit. Desktop variability remains a
recorded limitation, not an exclusion rule. This does not accept P07 or close T004.
