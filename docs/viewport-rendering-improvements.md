---
title: Viewport Rendering Improvement Experiments
doc_type: guide
status: active
owner: bcherrington
last_reviewed: 2026-08-13
---

# Viewport Rendering Improvement Experiments

## Purpose

This guide records the durable outcome of the Spec 009 viewport-rendering
exploration. It distinguishes useful diagnostic or branch-owner experiments
from behavior that is suitable for FreeCAD production use.

No Spec 009 experiment changes FreeCAD's production defaults. The native
renderer remains authoritative unless a developer explicitly enables the
session-local Rendering Experiments panel on the personal experiment branch.

## Outcome Map

| Candidate | Outcome | Durable decision |
| --- | --- | --- |
| Adaptive tessellation | Deferred | The probe demonstrates controllable remeshing, but production needs camera-driven invalidation plus triangle, memory, and time budgets. |
| Lighting and material presets | Deferred for core; retained for branch-owner evaluation | Presets are useful for visual comparison, but broad appearance policy and user-facing preset ownership need a separate design. |
| Depth-aware contrast | Implemented as an opt-in feature | At 15% it gave the clearest owner-approved improvement, especially for detail behind transparent bodies. The focused implementation is default-off and preserves native rendering when disabled or unsupported. |
| Topology-preserving screen-space edges | Deferred | The evaluator can suppress useful model edges and has not demonstrated a production-safe topology contract. |
| Weighted transparency | Rejected | Both blend-only evaluators produced incorrect representative-model transparency and were removed. Future work requires a shader-level weighted-fragment design. |
| Depth-precision clipping policy | Deferred | The evaluator worked on the compact mechanical model but produced poor results on a large BIM scene. It remains experimental only. |
| Hidden-line classification | Rejected | Both delayed-path implementations produced cross-object diagonal edge fans. The source and UI seams were removed; future work needs retained per-object geometry, transform, and document-depth state. |

## Depth-Aware Contrast Follow-up

Depth-aware contrast is the only Spec 009 candidate rebuilt on a clean feature
branch. It is available under **Edit → Preferences → Display → 3D View** as
**Depth-aware contrast**, with a **Strength** control from 0% to 100%. The
feature is off by default and the preferred evaluation strength is **15%**.

The same settings can be managed through
`BaseApp/Preferences/View/DepthAwareContrast` (Boolean, default `false`) and
`DepthAwareContrastStrength` (integer, default `15`). A live view redraw is
sufficient after either value changes; documents are not recomputed or
remeshed.

When enabled above 0%, the viewer renders document content into a private
color/depth target, applies bounded depth-derived contrast, and then renders
screen decorations once through the normal live path. It uses the configured
anti-aliasing request but may select a lower supported sample count. The
Navigation Cube, axis cross, dimensions, rubber-band selection, and other
screen overlays are not depth-processed. Unsupported OpenGL contexts or
allocation failures fall back to the native frame without retrying at idle.

Set `DepthAwareContrast` to `false` (or clear the preference checkbox) to roll
back immediately. Native rendering is also used at 0% strength. The feature
does not change saved-document data, object materials, transparency, selection
state, or camera state.

Focused Linux/X11 validation covers native/0%/15%, configured AA on and off,
fallback, selection and preselection, Navigation Cube isolation, resize and
HiDPI resource invalidation, multi-view switching, save/reopen, view teardown,
and idle resource counters. Before an upstream pull request is prepared, it
still needs:

- one linked user problem and GUI/rendering design consensus;
- Linux/X11 evidence plus Wayland, Windows, and macOS CI or explicitly accepted
  residual risk; and
- human-reviewed before/after images, code, tests, and AI-assistance disclosure.

The Spec 009 experiment branch is evidence for this follow-up, not an
implementation base. The focused branch should start from current
`upstream/main` and port only the depth-aware contrast behavior.

## Retained Experimental Value

The Rendering Experiments panel remains useful on the personal branch for
side-by-side assessment and regression discovery. Its controls are session
local, disabled by default, and not a supported user preference surface. Known
failed evaluators must not be reintroduced merely to complete the panel.

## Related Documents

- [BRep Edge Anti-Aliasing](brep-edge-antialiasing.md)
- [GPU Diagnostics](gpu-diagnostics.md)
- [Spec closure log](history/spec-closure-log.md)
