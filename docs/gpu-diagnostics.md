---
title: GPU Diagnostics
doc_type: guide
status: active
owner: bcherrington
last_reviewed: 2026-08-13
---

# GPU Diagnostics

## Purpose

FreeCAD's **Help -> GPU Diagnostics** command reports the graphics stack used by
the running application. Use it when viewport rendering differs from the
configured anti-aliasing setting, when comparing GPUs or display servers, or
when preparing a graphics issue report.

The command is diagnostic only. Opening it does not change preferences,
reconfigure OpenGL, or enable an anti-aliasing fallback.

## Design Authority

This guide is the durable design authority promoted from the closed
`006-gpu-diagnostics-msaa` package recorded in
[the spec closure log](history/spec-closure-log.md#2026-08-08---006-gpu-diagnostics-msaa).
The maintained implementation contract is:

- the developer harness links the production FreeCAD libraries and uses the
  production viewport, collector, dialog, classification, and serializers;
- requested-versus-actual anti-aliasing classification has one shared internal
  implementation rather than a separate test-only production API;
- `--auto-exit-ms` bounds dialog-mode harness runs even when OpenGL collection
  spans the shutdown deadline;
- the lifecycle contract is registered as an offscreen test and, when an X11
  display is available during configuration, an X11 test.

## Reading The Report

The summary distinguishes the requested anti-aliasing mode from the sample
buffers actually available to each live FreeCAD 3D viewport:

- **Active** means MSAA was requested and at least one live viewport reports
  active samples.
- **Inactive** means MSAA was requested but a live viewport reports zero active
  samples, or anti-aliasing was not requested.
- **Unknown** means no usable live viewport/context data was available.

For an MSAA problem, compare `preferences.requestedSamples` with each viewport's
`widgetFormatSamples`, `contextFormatSamples`, `glSampleBuffers`, and
`glSamples`. A requested value greater than zero together with zero actual
samples confirms that the preference did not produce a sampled viewport.

Line smoothing is reported separately. `glLineSmoothEnabled` and
`lineSmoothingBlendReady` do not prove that framebuffer MSAA is active.
Unavailable context-dependent values are exported as `null`; they must not be
interpreted as zero.

## Exporting A Report

The dialog can copy a human-readable report, copy JSON, or save JSON. The JSON
schema identifier is `org.freecad.gpu-diagnostics.v1` and its top-level objects
are:

| Field | Meaning |
| --- | --- |
| `freecad` | FreeCAD version. |
| `qt` | Qt version and active platform plugin. |
| `preferences` | Requested anti-aliasing and related rendering preferences. |
| `screens` | Display geometry, scale, DPI, and refresh data. |
| `viewports` | Per-viewport Qt format, OpenGL state, framebuffer, and renderer data. |
| `summary` | Interpreted requested/actual anti-aliasing status. |

Copy and save failures are shown in the dialog. Saving uses an atomic file
replacement so an incomplete report is not presented as successful output.

## Privacy And Issue Reports

Review an export before sharing it. Screen names, viewport object names,
display geometry, renderer strings, versions, and local configuration can help
identify a graphics problem, but may also disclose machine-specific details.
Redact identifiers that are not needed to reproduce the issue.

For a useful report, include the exported JSON, the requested anti-aliasing
setting, display server/platform, GPU selection or offload method, and a short
description of the visible result. Keep screenshots separate so they can be
reviewed and redacted independently.

## Limitations

The report describes the state observed when the dialog opens. It does not
repair missing sample buffers, benchmark rendering, or prove behavior on other
platforms. When multiple viewports are open, review each viewport entry rather
than relying only on the aggregate summary.

The developer-gated owned-framebuffer experiment that can compensate for a
zero-sample live widget is documented separately in
[BRep Edge Anti-Aliasing](brep-edge-antialiasing.md). GPU Diagnostics remains
observational and does not enable that path.

The broader candidate decisions and the focused depth-aware-contrast follow-up
are recorded in
[Viewport Rendering Improvement Experiments](viewport-rendering-improvements.md).
