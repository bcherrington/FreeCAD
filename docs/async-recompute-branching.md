---
title: Async recompute branch layering and validation
doc_type: design
status: active
owner: FreeCAD async recompute testers
last_reviewed: 2026-08-07
---

# Async Recompute Branch Layering And Validation

## Purpose

FreeCAD's experimental async recompute work is developed as independently
reviewable `async/*` source layers. `main` remains the upstream comparison
baseline and `personal/integration-testing` remains a merge-only combined-test
surface.

## Accepted Minimal Slice

The accepted minimal slice covers explicit `Std_Refresh` document recompute:

- async-enabled refresh queues work through the App async recompute API;
- async-disabled refresh retains the synchronous path;
- queued work exposes non-modal progress and cancellation feedback;
- callbacks report completion or `App::RecomputeFailure::Canceled` once; and
- closing a document removes queued work safely.

The implementation is split across `src/App/Application.*`,
`src/Gui/CommandDoc.cpp`, `src/Gui/AsyncRecomputeFeedback.*`, and their focused
App and Gui tests. Runtime threading and callback contracts remain documented
in `src/Doc/AsyncRecompute.dox`.

## Layering Contract

The validated source order is:

```text
main
`-- async/03-minimal-recompute-feedback
    `-- async/04-queued-recompute-cancel-callback
        `-- async/07-queued-recompute-user-cancel-foundation
            `-- async/09-refresh-cancel-feedback
                `-- async/11-refresh-cancel-feedback-validation
```

Each layer must either build independently or name the smallest predecessor
that owns its prerequisite. Fix conflicts in the owning source layer, then
merge tested layers into the integration branch. Do not author feature or test
changes directly on the integration branch.

The local integration selection is stored in
`.git/personal-integration-branches.json`. Because it is local `.git` metadata,
it is validation configuration rather than a tracked product artifact.

## Validation Contract

Build and run the focused App and Gui tests for changes to this slice:

```sh
cmake --build build/debug --target FreeCADApp FreeCADGui App_tests_run Gui_tests_run AsyncRecomputeFeedback_Tests_run -j2
build/debug/tests/App_tests_run --gtest_filter='AsyncRecomputeTest.*'
QT_QPA_PLATFORM=offscreen build/debug/tests/AsyncRecomputeFeedback_Tests_run -v1
```

When using CTest, use the CTest binary matching the CMake version that generated
the build tree. Also compare one known-slow explicit refresh with async enabled
and disabled, confirm that the progress UI is non-modal, and exercise queued
cancel/document-close behavior through the focused tests.

The 2026-08-07 validation used an 800-object document. `Std_Refresh` returned in
0.023 seconds with async enabled and displayed a live `Recompute` dialog with an
enabled Cancel control; the same command blocked for 1.747 seconds with async
disabled. Focused App, Gui, and registered CTest runs passed.

## Follow-Up Boundary

Part and PartDesign preview, accept/reject, property-panel, selection, undo/redo,
and running-work cancellation are not part of this minimal slice. Reopen those
areas only in a focused follow-up spec with explicit branch dependencies and
their own GUI validation matrix.
