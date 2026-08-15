---
title: Startup splash behavior and validation
doc_type: design
status: active
owner: FreeCAD Gui maintainers
last_reviewed: 2026-08-07
---

# Startup Splash Behavior And Validation

## Purpose

FreeCAD provides visible startup feedback through the unified
`src/Gui/SplashScreen.*` path before the main startup process completes. This
note records the durable behavior and the validation boundary that replaced the
historical separate `EarlySplash.*` design.

## Startup Order

`Gui::Application::runApplication()` resolves single-instance forwarding first,
sets up splash image paths, creates the unified splash when allowed, and then
runs `StartupProcess::execute()`. `StartupPostProcess` permits existing startup
dialogs to cover the splash, raises it again where appropriate, and closes it
after workbench activation or during exception cleanup.

The fallback `MainWindow::startSplasher()` path remains available when no
unified splash instance exists.

## Gating Contract

`Gui::SplashScreenOptions` and `Gui::shouldShowSplashScreen(...)` are the
focused policy seam. The unified splash is suppressed when:

- `ShowSplasher` is false;
- startup is hidden;
- run mode is not GUI;
- verbosity is strict; or
- the process only forwards to an existing instance.

The splash uses `SplashScreen::splashImage()` after image paths are configured,
with the compiled FreeCAD splash as fallback. Paint readiness is bounded
feedback, not an arbitrary minimum display delay.

## Validation

Run the focused gate suite after changing startup splash policy:

```sh
cmake --build build/debug --target FreeCADGui Gui_tests_run -j2
build/debug/tests/Gui_tests_run --gtest_filter='SplashScreen.*'
```

The 2026-08-07 run passed all five focused cases: normal GUI startup, disabled
splash, hidden startup, strict verbosity, and non-GUI mode. Exact-window X11
runtime inspection observed splash-type windows before the normal main window,
observed no splash when `ShowSplasher=false`, and observed no visible window for
hidden startup.

Commit `9fb6b7217b7` on `splash/03-startup-lifecycle-validation` adds a separate
offscreen Qt lifecycle test that exercises the real splash widget through
topmost startup, dialog yield, re-raise, pixmap sizing, and close. Its direct Qt
run and registered CTest entry passed on 2026-08-07.

Platform release validation remains appropriate for Wayland black/blank-frame
behavior, Windows disabled/second-instance startup, macOS Finder file-open,
custom artwork, HiDPI placement, and uncommon startup modal dialogs. Those are
platform-specific residual checks, not claims made by the Linux X11 run.
