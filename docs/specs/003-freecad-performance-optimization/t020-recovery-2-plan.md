---
title: T020 corrected native frame measurement campaign
doc_type: spec
artifact_type: review
status: in-progress
owner: local-developer
last_reviewed: 2026-11-07
---

# T020 Corrected Native Frame Measurement Campaign

This is D004's selected execution plan, recorded by Codex for local-developer
under the instruction to continue T020. Matched-arm qualification, all fourteen
checks against frozen contexts and six ownership controls pass. Independent
technical review found no blocking defect; this is not maintainer/human runtime
approval. No second-campaign acceptance samples have started. The first campaign remains incomplete; none of its partial observations
or inconclusive outcomes becomes eligible through this correction.

## Evidence For The Correction

Twelve diagnostic-only public PartDesign executions separated document open,
view setup, first qualifying swap and later event-dispatch completion. The old
endpoint waited until `processEvents()` returned before calling `glFinish`.
It included 6.355–10.789 ms after the first qualifying swap (median 8.664 ms).
These observations do not explain the first campaign's much larger timing spread.

A subsequent diagnostic reproduced `No active document subwindow` before capture.
The probe required keyboard-active MDI state immediately after opening a document.
Qt distinguishes keyboard-active and current subwindows; a fresh worker can own
one valid document window without that window being keyboard-active.
[Qt MDI documentation](https://doc.qt.io/qt-6/qmdiarea.html#currentSubWindow).
This establishes a harness failure class, not the cause of the lost first-campaign
worker diagnostics.

## Selected Probe Correction

The worker verifies a fresh empty App before open, that the GUI document matches
the opened document, and exactly one owned MDI subwindow with one native viewport.
It waits for window creation within a bounded event loop inside the open timer.
Keyboard activation does not determine ownership. Missing or ambiguous windows
remain failures. Qualification-only inactive/ambiguous cases cannot collect
acceptance observations.

The first qualifying native swap invokes a direct GUI-thread callback. It checks
the native context before `glFinish`, records the endpoint and resource snapshot,
and retains that exact accepted frame before Qt resumes. Completion errors are
re-thrown outside the callback. The natural VRML notification/readiness/render
generation barrier remains required.

The callback asserts `PartialUpdate`, which FreeCAD's Quarter viewport selects.
It resolves to one reusable raw RGBA8 renderbuffer without Qt framebuffer-helper
allocation. It restores every modified read/draw framebuffer, native read-buffer,
renderbuffer, scissor/sRGB state, previous context and surface. CPU readback/hash
runs outside event dispatch, saving/restoring read-buffer, pixel-pack buffer and
the six applicable 2D pack parameters. A bound pack buffer is explicitly unbound
before passing the CPU pointer. This follows Qt's post-composition swap and
preserved-framebuffer semantics.
[Qt OpenGL widget documentation](https://doc.qt.io/qt-6/qopenglwidget.html#frameSwapped).

The fixed CPU readback buffer is allocated/touched before the measurement window.
The fixed 1280×720 RGBA8 capture renderbuffer is allocated after the first frame's
endpoint/resource snapshot and reused during that worker. Later/final process RSS
includes observed driver/allocation effects. Neither its logical size nor RSS
establishes GPU residency. Image-plane decoded/Coin counters retain their reviewed
workload-owned logical-storage scope.

## Native Build Equivalence

Separate recovery worktrees independently copy each first-campaign arm's qualified
native outputs. For each arm, the entire `src` Git tree is identical to its origin;
the only changed tracked path is `tests/src/Gui/ImagePerformanceProbe.py`.
Generated/source inputs and all 2,433 copied native build outputs are checked by
hash; copied regular outputs have independent inodes. Dependencies remain pinned.
This is explicit reuse of previously compiled C++ outputs, not a recompilation or
an independent clean-build claim. The canonical configured paths are preserved
through the same read-only namespace wrapper. Both arms rerun focused correctness
and native/runtime qualification with the corrected common worker.

The sole baseline-to-change performance source delta remains the visibility guard
at the safe payload-ready restore barrier. Full resolution, calibration, payload
safety and all Phase 2 corrections remain required.

## New Execution Boundary

Before a second campaign:

1. Complete all four GUI and three command qualifications on both recovery arms,
   including independent texture/frame/reference checks and ownership controls.
2. Freeze exact corrected worker, new context/profile/configuration, namespace,
   source/build/output/runtime-resource, environment, fixture and host identities.
3. Freeze the failure collector extension and an exclusive per-execution
   diagnostic destination. Its post-failure capture cannot authorize a retry.
4. Record D004's dated owner disposition and independently review the concrete
   freeze. No unresolved qualification defect may be waived.

All seven cells then start from pair zero under the original fixed schedule:
one diagnostic pair, three warmup pairs, twenty measured alternating pairs. There
is one attempt per cell. No partial first-campaign observations are reused or
combined. All affected cells are recollected; failed or noisy first-campaign
results stay failed/ineligible/inconclusive in their original record.

The original D001 plan, protocol, thresholds, percentile/bootstrap rules, fixture
identities, warmed-filesystem policy, native X11, viewport, DPR and eight-sample
MSAA settings remain unchanged. There is no host-wide power-policy change or
termination of unrelated applications. Record competing load and apply the
original variability rule. Any new failure or uncertainty stays visible; this
plan promises no performance pass.
