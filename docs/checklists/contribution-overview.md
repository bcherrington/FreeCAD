---
title: FreeCAD contribution overview
doc_type: checklist
status: active
owner: local-developer
last_reviewed: 2026-05-18
---

# FreeCAD Contribution Overview

## Purpose

This note summarizes the contribution rules and checks to use for local work in
this repository. Treat `CONTRIBUTING.md` and the online FreeCAD documentation as
the authoritative sources.

## Scope

This checklist covers local contribution hygiene, UI/icon expectations, and
pre-submission gates for FreeCAD changes in this checkout. It does not replace
upstream policy, code review, or maintainer guidance.

## Completion Criteria

A contribution is ready for review when the relevant local gates pass or their
limitations are documented, the branch stays focused on one problem, and the PR
description clearly reports behavior changes and validation.

## Where To Find More Information

- Local process: `CONTRIBUTING.md`
- Local coding style: `.clang-format`, `.pre-commit-config.yaml`, nearby source
  files
- Build and development workflow: `README.md`, `pixi.toml`, and the FreeCAD
  Developers Handbook
- License requirements: `LICENSE`, `CONTRIBUTING.md`
- Community conduct: `CODE_OF_CONDUCT.md`
- Online handbook: <https://freecad.github.io/DevelopersHandbook/>
- Upstream contribution process:
  <https://github.com/FreeCAD/FreeCAD/blob/main/CONTRIBUTING.md>

## Checklist

- [ ] The branch is based on current `main`.
- [ ] The change addresses one issue or clearly scoped problem.
- [ ] No unrelated formatting, generated files, or local artifacts are included.
- [ ] New source files have the correct license/SPDX treatment.
- [ ] User-visible strings are translatable where appropriate.
- [ ] Existing Python APIs and add-on behavior are preserved, or breaking
  changes are explicitly documented.
- [ ] The project builds, or any build limitation is documented.
- [ ] Focused tests were run for the touched area.
- [ ] `pixi run test` or an equivalent broader test pass was run when practical.
- [ ] `pre-commit run --all-files` passes, or any failure is documented.
- [ ] UI changes include manual verification notes and visual evidence for
  review.
- [ ] The PR description links the issue, explains the implementation, lists
  tests, and calls out behavior or API changes.

## Contribution Expectations

FreeCAD expects pull requests to solve one discrete, well-defined problem. Keep
changes minimal, avoid unrelated refactors, and do not add dependencies unless
there is no reasonable alternative. Each commit should compile cleanly and have a
succinct message explaining what it achieves.

For UI work, preserve existing workflows unless the issue explicitly requires a
change. The PR description must explain the UI change and should include
screenshots, screen recordings, or clear before/after notes.

Do not submit raw AI output. Any AI-assisted work must be reviewed, validated,
and explainable by the contributor.

## UI And Icon Guidelines

For UI changes, preserve existing workflows unless the issue explicitly requires
a change. The PR description must explain the user-facing behavior and should
include screenshots, screen recordings, or clear before/after notes.

New or modified icons should follow the FreeCAD icon art guide:
<https://github.com/obelisk79/IconArtGuide>. Use a 24 px canvas, respect the
2 px protected perimeter, keep the functional area simple, and avoid using the
FreeCAD logo inside command icons. Prefer a gray base plus one action color.
Use palette colors already common in FreeCAD icons, avoid color as the only
meaning cue, and keep strokes close to the guide's 1 px outline / lightweight
defining-line rules. Register new icons in `src/Gui/Icons/resource.qrc`.

## Local Gates

Use the pixi tasks when available:

```sh
pixi run initialize
pixi run configure
pixi run build
pixi run test
pixi run freecad
```

Direct equivalents are:

```sh
cmake --preset conda-linux-debug
cmake --build build/debug
ctest --test-dir build/debug
```

Run pre-commit before submission:

```sh
pre-commit run --all-files
```

The configured hooks check whitespace, final newlines, YAML, large files, line
endings, Python formatting with Black, C++ formatting with clang-format, and
version synchronization.

## Notes For Issue 28879

For the Python Console standalone window work, keep the first PR focused on the
Python Console. Preserve the existing docked `View > Panels > Python Console`
behavior, avoid a generic dock-window rewrite, and treat Report View standalone
support as a follow-up unless maintainers request it in the same change.
