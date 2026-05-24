# Repository Guidelines

## Project Structure & Module Organization

FreeCAD is a large C++/Python Qt application. Core application and GUI code live
under `src/`, especially `src/App`, `src/Base`, `src/Gui`, and `src/Main`.
Workbench-specific code is organized under `src/Mod/<Workbench>/`, often split
into `App` and `Gui` subdirectories. Tests live in `tests/` and `tests/src/`,
with additional module-specific tests under `src/Mod/Test` and individual
workbench trees. Assets, translations, examples, packaging, and build helpers
are in `data/`, `package/`, `cMake/`, `tools/`, and `.github/`.

## Build, Test, and Development Commands

Use pixi tasks for normal development; they wrap the common CMake workflow.

- `pixi run initialize`: initialize git submodules.
- `pixi run configure`: configure a debug build in `build/debug`.
- `pixi run build`: build the debug target.
- `pixi run install`: install the debug build.
- `pixi run test`: run `ctest --test-dir build/debug`.
- `pixi run freecad`: launch `build/debug/bin/FreeCAD`.

Use `pixi run build` for build verification unless the user explicitly asks for
a raw CMake command or a lower-level diagnostic requires it. Direct CMake
equivalents are `cmake --preset conda-linux-debug`,
`cmake --build build/debug`, and `ctest --test-dir build/debug`.

## Async Recompute Branch Workflow

Async recompute work for
`../FreeCAD-docs/docs/specs/29757-async-recompute-branching/` must stay off
`main`. Keep `main` as a clean upstream comparison branch and put implementation
work on named `async/*` layer branches.

`personal/integration-testing` is a merge-only integration branch. Do not make
feature, test, or documentation edits directly on that branch. Create a source
branch first, make and validate the change there, then merge the source branch
into `personal/integration-testing`.

New independent async layers should branch from `upstream/main`. If a layer
depends on an earlier async layer, branch from the documented predecessor
instead and record that dependency here before merging it into the integration
branch.

Current minimal responsiveness layer:

- Branch: `async/03-minimal-recompute-feedback`
- Commit: `8d65ac7968f Gui: show feedback for async document recompute`
- Scope: coarse GUI feedback for explicit async `Std_Refresh` document
  recompute, plus canceled callback cleanup for queued recompute requests.
- Files changed by this layer:
  - `src/App/Application.cpp`
  - `src/App/Application.h`
  - `src/Gui/CommandDoc.cpp`

Current cancellation coverage layer:

- Branch: `async/04-queued-recompute-cancel-callback`
- Commit: `288c673babf App: cover queued async recompute cancellation`
- Scope: App-level regression coverage proving that queued async recompute
  requests canceled during document close report
  `App::RecomputeFailure::Canceled` through their callback exactly once.
- Files changed by this layer:
  - `tests/src/App/AsyncRecompute.cpp`

Current queued cancellation foundation layer:

- Branch: `async/07-queued-recompute-user-cancel-foundation`
- Commit: `22195ee5669 App: expose queued async recompute cancellation`
- Scope: App-level cancellation API for queued async recompute requests that
  reports `App::RecomputeFailure::Canceled` through callbacks without waiting
  for unrelated in-flight recomputes.
- Files changed by this layer:
  - `src/App/Application.cpp`
  - `src/App/Application.h`
  - `tests/src/App/AsyncRecompute.cpp`

Current refresh cancel feedback layer:

- Branch: `async/09-refresh-cancel-feedback`
- Commit: `e248a2f0e3b Gui: allow canceling queued async refresh`
- Scope: explicit `Std_Refresh` progress feedback now exposes a Cancel button
  that removes queued recompute requests through
  `cancelQueuedRecomputeRequestsForDocument(...)`; if recompute is already
  running, the button is disabled and progress remains visible.
- Files changed by this layer:
  - `src/Gui/CommandDoc.cpp`

Current refresh cancel feedback validation layer:

- Branch: `async/11-refresh-cancel-feedback-validation`
- Commit: `9f5c255df1a Gui: test async refresh cancel feedback`
- Scope: extracted async refresh cancel feedback into a Gui helper and added
  offscreen Qt validation for queued cancel feedback and already-running
  feedback states.
- Files changed by this layer:
  - `src/Gui/AsyncRecomputeFeedback.cpp`
  - `src/Gui/AsyncRecomputeFeedback.h`
  - `src/Gui/CMakeLists.txt`
  - `src/Gui/CommandDoc.cpp`
  - `tests/src/Gui/AsyncRecomputeFeedback.cpp`
  - `tests/src/Gui/CMakeLists.txt`

Expected layering:

```text
main
`-- async/02-explicit-refresh-async-recompute
    `-- async/03-minimal-recompute-feedback
        `-- async/04-queued-recompute-cancel-callback
            `-- async/07-queued-recompute-user-cancel-foundation
                `-- async/09-refresh-cancel-feedback
                    `-- async/11-refresh-cancel-feedback-validation
```

If `async/02-explicit-refresh-async-recompute` exists and owns the explicit
refresh routing, rebase or recreate `async/03-minimal-recompute-feedback` on top
of it before integration. In this checkout, the explicit `Std_Refresh` async
routing was already present on `main`, so `async/03-minimal-recompute-feedback`
was created from `main`.

Use integration branches only for combined testing. Do not make direct feature
edits on `personal/integration-testing`, `pr-29757-async-freecad-gui`, or
`main`. Merge tested layer branches into an integration branch instead.

This repo currently has no `pixi run integration` or `pixi run intergation`
task. The local integration branch manifest is:

```text
.git/personal-integration-branches.json
```

For future local integration rebuilds, keep
`async/03-minimal-recompute-feedback` and
`async/04-queued-recompute-cancel-callback` and
`async/07-queued-recompute-user-cancel-foundation` and
`async/09-refresh-cancel-feedback` and
`async/11-refresh-cancel-feedback-validation` listed in the `branches` array of
that manifest. Because the manifest is under `.git/`, it is local metadata, not
a tracked source file.

Focused validation used for this layer:

```sh
cmake --build build/debug --target FreeCADApp FreeCADGui App_tests_run Gui_tests_run -j2
ctest --test-dir build/debug -R '^AsyncRecomputeTest\.' --output-on-failure
build/debug/tests/Gui_tests_run --gtest_filter='*'
cmake --build build/debug --target FreeCADGui AsyncRecomputeFeedback_Tests_run -j2
ctest --test-dir build/debug -R '^(AsyncRecomputeFeedback_Tests_run|AsyncRecomputeTest\.)' --output-on-failure
```

## Coding Style & Naming Conventions

Follow nearby code first. C++ is formatted by `.clang-format`; Python is
formatted by Black with line length 100. Run pre-commit hooks before submitting:

```sh
pre-commit run --all-files
```

Prefer descriptive class and file names matching existing FreeCAD patterns, for
example `MainWindow`, `DockWindowManager`, `TaskView`, and
`src/Mod/PartDesign/Gui/Workbench.cpp`.

## Testing Guidelines

Builds and tests should pass for the affected target. Use focused CTest runs
while iterating, then broaden before review:

```sh
ctest --test-dir build/debug -R <test-name>
pixi run test
```

Add or update tests for behavior changes, especially shared GUI, document model,
geometry, or workbench command logic. Include manual verification notes for UI
changes that are difficult to automate.

## Commit & Pull Request Guidelines

Keep each PR focused on one problem. Commits should compile independently and
use succinct imperative subjects, such as `Gui: preserve dock overlay state`.
Squash checkpoint commits before review.

PRs must describe the change, link relevant issues, list test results, and call
out API or behavior changes. UI changes should include screenshots or a clear
before/after description. Avoid new dependencies unless justified.

## Agent-Specific Instructions

This repository is large. Prefer targeted searches such as `rg "SymbolName"`
and scoped reads over full-tree exploration. Do not rewrite unrelated files or
reformat broad areas while making localized changes.
