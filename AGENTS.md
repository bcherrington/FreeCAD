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

Use pixi tasks when available; they wrap the common CMake workflow.

- `pixi run initialize`: initialize git submodules.
- `pixi run configure`: configure a debug build in `build/debug`.
- `pixi run build`: build the debug target.
- `pixi run install`: install the debug build.
- `pixi run test`: run `ctest --test-dir build/debug`.
- `pixi run freecad`: launch `build/debug/bin/FreeCAD`.

Direct CMake equivalents are `cmake --preset conda-linux-debug`,
`cmake --build build/debug`, and `ctest --test-dir build/debug`.

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
