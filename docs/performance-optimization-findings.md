---
title: Performance optimization findings and routing
doc_type: research
status: active
owner: FreeCAD performance investigators
last_reviewed: 2026-08-07
---

# Performance Optimization Findings And Routing

## Recommendation

Do not continue the original performance umbrella as a source-work owner. Its
remaining value is historical: it explains why several restore and image
experiments were attempted and routes any renewed work to narrower owners with
fresh measurements.

## Current Source Reconciliation

| Area | Current disposition |
| --- | --- |
| Included-file restore | Buffered restore remains in `src/App/PropertyFile.cpp`; use a branch-specific readiness record before upstream submission. |
| Image-plane lifecycle | Deferred/lazy full-resolution restore remains in `src/Gui/ViewProviderImagePlane.cpp`; any visual behavior change needs focused GUI evidence. |
| Tree status startup churn | Restore-time batching remains in `src/Gui/Tree.cpp`; wider tree work needs new document-open traces. |
| Bitmap conversion | The original local approach is superseded by newer upstream work in commit `7252af06fb89`; do not revive the old branch unchanged. |
| Element-map parsing | The historical parser helper is not in current `src/App/ElementMap.cpp`; treat it as an investigation lead, not implemented current behavior. |
| GUI document restore | Owned by Spec 004's document-open investigation, not by the umbrella. |
| Upstream preparation | Owned by Spec 002's per-branch readiness records. |

## Reopening Rule

Reopen a performance area only with a focused problem statement, a current
upstream baseline, reproducible before/after measurements, correctness tests,
and a branch-specific validation plan. Historical debug timings are useful for
triage but are not fresh performance acceptance evidence.
