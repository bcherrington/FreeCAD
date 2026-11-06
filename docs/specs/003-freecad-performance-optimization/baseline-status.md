---
title: Spec 003 implementation baseline
doc_type: spec
artifact_type: reference
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Implementation Baseline

## Evidence Boundary

This inventory records inspected existing implementation and imported dated
results. It is not the current execution queue, a record of source work begun in
this session, or acceptance of the reviewed implementation. Source observations
come from the pre-replan package and the MoE review at root c69560af8a/docs
8d1abe9; the T019 manifest pins the later intake composition. Existing binaries
remain ineligible for new acceptance until tied to actual build provenance.

The five old `[/]` parent statuses mixed implementation presence with unfinished
current qualification. The replan moves those facts here and leaves the five
remaining validation checkpoints pending. No completed subtask/current evidence
is discarded: all their existing children were pending, and original observations
and historical files remain available. Reserve partial execution status for a
future task with completed, evidence-bearing portions of its own acceptance.

## Included-file restore

- Source touchpoints: `src/App/PropertyFile.cpp; src/Base/Reader.cpp`.
- Imported observation: Buffered copy/error checks are present; imported Nov 4 tests and scratch roundtrips cover a subset, not all compatibility criteria.
- Remaining acceptance: F006; archive-level errors/recovery need explicit policy and current fixtures.
- Current executable checkpoint: T002 — Verify included-file restore compatibility on the target revision.
- Current qualifying runtime result: none recorded for the expanded acceptance.

## Bitmap conversion

- Source touchpoints: `src/Gui/BitmapFactory.cpp`.
- Imported observation: Four direct-format tests exist; full fallback/rendered visual matrix remains open.
- Remaining acceptance: F001; extent/allocation safety and the full current format/rendered matrix remain open.
- Current executable checkpoint: T003 — Complete bitmap and rendered image correctness coverage.
- Current qualifying runtime result: none recorded for the expanded acceptance.

## Element-map parsing

- Source touchpoints: `src/App/ElementMap.cpp`.
- Imported observation: string_view tokenizer and parser regression test inspected; Nov 4 focused result is imported evidence, not a fresh run.
- Remaining acceptance: F004; bounded numeric grammar, writer forms, aliases/shared children and CAD target identity remain open.
- Current executable checkpoint: T005 — Verify element-map parser compatibility.
- Current qualifying runtime result: none recorded for the expanded acceptance.

## Image-plane lifecycle

- Source touchpoints: `src/Gui/ViewProviderImagePlane.cpp; src/Gui/Document.cpp`.
- Imported observation: Pending texture, finishRestoring and show paths inspected; historical GUI smoke does not close the full matrix.
- Remaining acceptance: F002; stored calibration can be overwritten when deferred loading follows cleared flags.
- Current executable checkpoint: T006 — Validate deferred full-resolution image-plane lifecycle.
- Current qualifying runtime result: none recorded for the expanded acceptance.

## Tree status batching

- Source touchpoints: `src/Gui/Tree.cpp; src/Gui/Tree.h`.
- Imported observation: Single pending status update exists; per-object delete side effects are documented; larger-model coverage remains open.
- Remaining acceptance: F003/F005; receiver lifetime and completion coverage need corrections and current qualification.
- Current executable checkpoint: T007 — Validate tree status batching and larger-model behavior.
- Current qualifying runtime result: none recorded for the expanded acceptance.

## Completed Preparation

T021 — Expose lifecycle decisions and verify execution guardrails and T019 — Pin
candidate validation revisions and evidence applicability remain complete with
their original evidence. Their completion covers documentation/provenance only.
The immutable [T019 snapshot](validation-manifest.md) and JSON remain unchanged
by this replan. Current decision status is in [open-decisions.md](open-decisions.md).

## Historical Inputs

The old spec/plan/research/checklist and imported 2026-11-04 verification retain
their dated historical roles. Source implementation presence, historical test
passes and static defect findings are separate evidence classes; none alone
completes the remaining correction/qualification tasks.

## Related Artifacts

- [Execution plan](tasks.md#execution-order)
- [Requirements](requirements.md)
- [Finding and evidence ledger](verification.md)
- [Traceability](traceability.md)
