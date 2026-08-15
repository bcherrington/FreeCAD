# Spec Closure Log

## Entries

### 2026-08-14 - 015-compact-view-rails-rework

- **Spec:** `docs/specs/compact-view-rails-rework/spec.md`
- **Title:** JetBrains-style panel rails and placement Requirements
- **Final legacy spec commit:** `838de8f6ea2`
- **Implementation commit:** `d84e85757e7`
- **Closure cleanup commit:** pending reconciliation after the cleanup commit
- **Closure action:** removed
- **Durable docs updated:**
  - `docs/design/jetbrains-style-compact-ui.md`
- **Verification summary:** Closure validation not yet executed.
- **Residual risks:**
  - none
- **Follow-up:** none
### 2026-08-14 - 013-compact-ui-shortcut-preservation

- **Spec:** `docs/specs/013-compact-ui-shortcut-preservation/`
- **Title:** Compact UI Shortcut Preservation Requirements
- **Final spec commit:** not retained in this branch history
- **Closure cleanup commit:** not retained in this branch history
- **Closure action:** removed
- **Durable docs updated:**
  - `docs/design/jetbrains-style-compact-ui.md`
- **Verification summary:** Lifecycle lint, task-state audit, evidence-quality,
  and closure-check passed; `FreeCADGui` rebuilt successfully; focused compact
  chrome Qt validation passed both the direct QtTest target and the focused
  CLion `ctest` run; selected-file pre-commit and diff hygiene checks passed;
  an isolated Linux/X11 safe-mode smoke confirmed the compact chrome starts
  with the native menu bar hidden.
- **Residual risks:**
  - Wayland, Windows, and macOS shortcut routing remain unverified.
  - Live GUI smoke covered Linux/X11 safe mode only; shortcut correctness on
    other platforms still relies on the deterministic Qt regressions.
- **Follow-up:** Revalidate the compact shortcut-host lifecycle if the compact
  menu synchronization or workbench-activation seams change again.

### 2026-08-07 - 005-async-recompute-branching

- **Spec:** `docs/specs/005-async-recompute-branching/`
- **Legacy ID:** `29757-async-recompute-branching`
- **Title:** Async Recompute Branch Layering Requirements
- **Final spec commit:** not retained in this branch history
- **Closure cleanup commit:** `28b6dd96b3d`
- **Closure action:** removed
- **Durable docs updated:**
  - `docs/async-recompute-branching.md`
  - `src/Doc/AsyncRecompute.dox`
- **Verification summary:** Lifecycle closure-check reported `ready: true` with
  no blockers; focused App and Gui async tests passed; an 800-object X11 A/B
  showed non-blocking async refresh with progress versus blocking synchronous
  refresh; independent review accepted the branch, source, test, and routing
  claims.
- **Residual risks:**
  - Part and PartDesign preview, accept/reject, property-panel, selection,
    undo/redo, and running-work cancellation remain outside the completed
    minimal explicit-refresh slice.
- **Follow-up:** Open a focused future spec before implementing the routed
  broader async behaviors.

### 2026-08-07 - 003-splashscreen-startup

- **Spec:** `docs/specs/003-splashscreen-startup/`
- **Legacy ID:** `16264-splashscreen-startup`
- **Title:** Splashscreen Startup Requirements
- **Final spec commit:** not retained in this branch history
- **Closure cleanup commit:** `6963fb3dcaa`
- **Closure action:** removed
- **Durable docs updated:**
  - `docs/startup-splash.md`
- **Verification summary:** Lifecycle closure-check reported `ready: true` with
  no blockers; five gating tests and the new offscreen splash lifecycle test
  passed; X11 default, disabled, and hidden startup were exercised; independent
  source review accepted dialog-yield, re-raise, close, and image-path seams.
- **Residual risks:**
  - Wayland, Windows, macOS, custom-artwork, HiDPI, and uncommon modal-dialog
    behavior remain target-platform release validation.
- **Follow-up:** Revalidate the routed platform matrix when startup splash code
  or artwork changes.

### 2026-08-07 - 001-freecad-performance-optimization

- **Spec:** `docs/specs/001-freecad-performance-optimization/`
- **Legacy ID:** `freecad-performance-optimization`
- **Title:** FreeCAD Performance Optimization Requirements
- **Final spec commit:** not retained in this branch history
- **Closure cleanup commit:** `6ca5cd9de9e`
- **Closure action:** removed
- **Durable docs updated:**
  - `docs/performance-optimization-findings.md`
  - `docs/specs/002-upstream-pr-readiness/`
  - `docs/specs/004-performance-investigation/`
- **Verification summary:** Lifecycle closure-check reported `ready: true` with
  no blockers; current-source and branch reconciliation distinguished retained,
  superseded, and historical-only slices; independent review accepted retiring
  the umbrella as a source-work owner.
- **Residual risks:**
  - Historical debug timings are not current performance acceptance evidence.
- **Follow-up:** Reopen only a focused owner with a current upstream baseline,
  reproducible measurements, correctness tests, and branch-specific validation.

### 2026-08-07 - 008-compact-ui-idle-cpu-loop

- **Spec:** `docs/specs/008-compact-ui-idle-cpu-loop/`
- **Legacy ID:** `20260607-compact-ui-idle-cpu-loop`
- **Legacy package path:** `docs/specs/20260607-compact-ui-idle-cpu-loop/`
- **Title:** Compact UI Idle CPU Loop Requirements
- **Final spec commit:** not retained in this branch history
- **Closure cleanup commit:** not retained in this branch history
- **Closure action:** removed
- **Durable docs updated/current successor:**
  - `docs/design/jetbrains-style-compact-ui.md`
- **Historical closure destination:** The original root-level guide was
  `docs/jetbrains-style-compact-ui.md`; it was later removed by `068a7c8` and
  superseded by the current design path above.
- **Verification summary:** Package lint passed with 0 errors and 0 warnings;
  closure readiness passed with all five must-have requirements complete; all
  15 task and verification evidence records were classified concrete.
- **Residual risks:**
  - Pointer-driven frameless interaction remains target-platform release
    validation, owned by the FreeCAD GUI maintainer or release tester and
    documented in `docs/design/jetbrains-style-compact-ui.md`.
- **Follow-up:** No blocking follow-up.

### 2026-08-08 - 006-gpu-diagnostics-msaa

- **Spec:** `docs/specs/006-gpu-diagnostics-msaa/`
- **Legacy ID:** `20260606-gpu-diagnostics-msaa`
- **Title:** GPU Diagnostics and Viewport MSAA Reliability Requirements
- **Final spec commit:** not retained in this branch history
- **Closure cleanup commit:** `eea1a774ffe`
- **Closure action:** removed
- **Durable docs updated:**
  - `docs/gpu-diagnostics.md`
  - `docs/brep-edge-antialiasing.md` (eventual Spec 007 promotion target)
- **Verification summary:** Lifecycle closure-check reported `ready: true` with
  no blockers; five focused diagnostics tests passed; Intel and NVIDIA X11
  harness runs, repeated bounded exits, and one full-FreeCAD dialog smoke
  passed; repository hooks, focused builds, lifecycle lint, and independent
  review completed.
- **Residual risks:**
  - Wayland, Windows, and macOS runtime behavior remains untested.
  - Clipboard and atomic-save warning paths were source-reviewed but not
    exercised through injected failures.
  - The diagnostic slice reports missing MSAA but does not repair it.
- **Follow-up:** Completed by Spec 007; the durable renderer decision and
  opt-in owned-FBO guidance now live in `docs/brep-edge-antialiasing.md`.

### 2026-08-09 - 007-brep-edge-aa-experiments

- **Spec:** `docs/specs/007-brep-edge-aa-experiments/`
- **Legacy ID:** `20260606-brep-edge-aa-experiments`
- **Title:** BRep Edge Anti-Aliasing Investigation and Experiments Requirements
- **Final spec commit:** not retained in this branch history
- **Closure cleanup commit:** `eea1a774ffe`
- **Closure action:** removed
- **Durable docs updated:**
  - `docs/brep-edge-antialiasing.md`
  - `docs/gpu-diagnostics.md`
- **Verification summary:** Lifecycle closure-check reported `ready: true` with
  no blockers; source commit `531bb0e0211` passed the affected build, five GPU
  Diagnostics tests, all 23 bounded harness tests, selected-file pre-commit,
  CRLF-aware source/docs diff checks, and high-signal changed-line clang-tidy.
  Independent maintainer-style review accepted the opt-in slice after its
  sample-downgrade blocker, fallback-transition gap, and evidence-consistency
  request were fixed.
- **Residual risks:**
  - Runtime evidence remains Linux/X11/Mesa only; Wayland, Windows, and macOS
    are unverified.
  - Performance evidence uses a synthetic 100-cube scene in a Debug build and
    is not a release budget.
  - Forced post-allocation GL failure and cached-failure suppression remain
    source-reviewed rather than injected to avoid a production test seam.
  - Default or upstream use still requires a linked issue, rendering/GUI
    consensus, representative models, and cross-platform evidence.
- **Follow-up:** Reopen a focused future spec before adding a user preference,
  changing defaults, or proposing the renderer upstream.

### 2026-08-13 - 009-viewport-rendering-improvements

- **Spec:** `docs/specs/009-viewport-rendering-improvements/`
- **Title:** Viewport Rendering Improvements Requirements
- **Final spec commit:** not retained in this branch history
- **Closure cleanup commit:** `eea1a774ffe`
- **Closure action:** removed
- **Durable docs updated:**
  - `docs/viewport-rendering-improvements.md`
  - `docs/brep-edge-antialiasing.md`
  - `docs/gpu-diagnostics.md`
- **Verification summary:** Lifecycle lint and task-state audit passed with no
  findings; closure readiness reported `ready: true` with no blockers; the
  focused X11 normal-FreeCAD module passed 8/8 in 33.842 seconds; `FreeCADGui`
  rebuilt successfully; selected-file hooks and repository-pinned formatting
  passed. Host `clang-tidy` could not produce a valid result because the build
  compilation database omitted both touched translation units and the host
  analyzer crashed in one third-party-header check; no clean analyzer pass is
  claimed. Maintainer-style review approved exploration closure but classified
  the mixed branch as not ready for an upstream PR.
- **Residual risks:**
  - Depth-aware contrast release cost, large-model interaction behavior,
    Wayland, Windows, and macOS remain unverified.
  - Adaptive tessellation, lighting/material presets, topology-aware edges,
    and depth precision remain production-deferred.
  - Weighted transparency and hidden-line classification were rejected; the
    broken hidden-line implementation was removed.
- **Follow-up:** Implement only depth-aware contrast, with the preferred 15%
  strength, on a clean branch from current `upstream/main`; validate it in
  `personal/integration-testing` before any upstream PR preparation.

### 2026-08-13 - 012-depth-aware-contrast

- **Spec:** `docs/specs/012-depth-aware-contrast/`
- **Title:** Depth-Aware Viewport Contrast Requirements
- **Final spec commit:** not retained in this branch history
- **Closure cleanup commit:** `eea1a774ffe`
- **Closure action:** removed
- **Durable docs updated:**
  - `docs/viewport-rendering-improvements.md`
- **Verification summary:** Lifecycle lint and task-state audit passed with no
  findings and closure readiness reported `ready: true`. The focused Debug
  module passed 11 tests with one environment-specific skip; the separately
  forced OpenGL fallback and HiDPI cases passed. Release fixed-camera review
  passed on 91- and 148-object documents. Median/p95 overhead remained within
  the 20% personal-branch budget, and settled idle windows produced no feature
  redraw or allocation attempts. Selected-file hooks, formatting, builds,
  static analysis with recorded advisories, and independent review completed.
- **Residual risks:**
  - Wayland, Windows, macOS, and software OpenGL remain unverified.
  - Upstream preparation still requires a linked issue or DWG/design consensus,
    a normal CI fallback lane, cross-platform evidence, and human PR review.
  - Full-translation-unit Clang 18 reports advisory complexity and OpenGL API
    array/cast findings in the focused renderer path.
- **Follow-up:** Continue personal testing through integration merge
  `a4dc9a148b7`; keep the feature default-off and use native fallback on failure.
