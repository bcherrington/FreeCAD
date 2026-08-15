---
title: Upstream PR readiness research
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: research
---

# Research

## Package Identity

- Current package: `docs/specs/002-upstream-pr-readiness/`
- Legacy package ID: `upstream-pr-readiness`

## Review-Cycle Baseline

- `git fetch upstream main` completed on `2026-08-07`.
- Review-cycle baseline ref: `upstream/main`
- Review-cycle baseline commit:
  `7937058dededc800dd0f3197185daf75d146c9d6`
- Review-cycle baseline subject:
  `Make it possible to invoke multiple tests at once (#31402)`

This package should be refreshed again before any actual branch cleanup or PR
drafting starts if `upstream/main` has moved.

## Current Contribution Policy Baseline

### Binding Local Repository Files

- `CONTRIBUTING.md` currently requires minimal, single-problem PRs; clean
  history; passing self-tests on target platforms; attribution for cherry-picked
  work; and AI-assisted work that remains fully human-reviewed and attributable.
- `.github/pull_request_template.md` currently requires the contributor to
  affirm that the PR is not unverified AI output and reminds contributors to
  include issue links and before/after images for GUI changes.
- `AI_POLICY.md` currently requires human ownership, disclosure of AI
  assistance, and PR/commit communication by the contributor rather than the
  tool.

### Official External Guidance Consulted On 2026-08-07

- [FreeCAD Developers Handbook: Getting Started](https://freecad.github.io/DevelopersHandbook/gettingstarted/)
  - Used for the public PR submission and review-process guidance.
- [FreeCAD Developers Handbook: Maintainers Guide](https://freecad.github.io/DevelopersHandbook/maintainersguide/)
  - Used for merge-meeting and maintainer review expectations.

### Current Policy-Discussion Watch Items

- [PR #30268: Contributing: Improve the AI policy](https://github.com/FreeCAD/FreeCAD/pull/30268)
  - Checked on `2026-08-07`.
  - GitHub currently shows the PR as `Open`.
  - The discussion includes moving AI policy text into `AI_POLICY.md` and
    updating the PR template checkbox wording.
- [PR #30273: Contribution Standards Improve Rule, mostly around AI discussions](https://github.com/FreeCAD/FreeCAD/pull/30273)
  - Checked on `2026-08-07`.
  - GitHub currently shows the PR as `Open`.
  - The discussion explicitly notes a conflict with PR `#30268`.

Current package rule: treat the merged local files as binding, and treat these
two GitHub PRs as watch items that may change later PR framing but do not block
this package revision.

## Canonical Candidate Inventory

The legacy package mixed stale names, canonical branch refs, and composite local
branches. The table below is the current working set as of `2026-08-07`.

| Legacy package reference | Canonical current ref | Ahead of `upstream/main` | Behind `upstream/main` | Branch-shape note | Current disposition | Record |
|--------------------------|-----------------------|---------------------------|------------------------|-------------------|--------------------|--------|
| `toolbar-persistence-save-settings` | `toolbar-persistence-save-settings` | 1 | 990 | single functional commit; no docs/spec contamination | `candidate-clean` | [record](readiness-records/toolbar-persistence-save-settings.md) |
| `overlay-tab-destruction-cleanup` | `overlay-tab-destruction-cleanup` | 2 | 946 | one functional commit plus one merge from `main` | `candidate-rebuild` | [record](readiness-records/overlay-tab-destruction-cleanup.md) |
| `perf-image-plane-lifecycle-lazy-texture` | `perf/image-plane-lifecycle-lazy-texture` | 7 | 946 | two functional commits plus four local docs/spec commits and one merge | `candidate-rebuild` | [record](readiness-records/perf-image-plane-lifecycle-lazy-texture.md) |
| `perf-tree-status-startup-followup` | `perf/tree-status-startup-followup` | 6 | 990 | one functional commit plus five local docs/spec commits | `candidate-rebuild` | [record](readiness-records/perf-tree-status-startup-followup.md) |
| `perf-included-file-restore-buffering` | `perf/included-file-restore-buffering` | 2 | 948 | one functional restore commit plus one merge from `main` | `candidate-rebuild` | [record](readiness-records/perf-included-file-restore-buffering.md) |
| `perf-element-map-restore-parsing` | `perf/element-map-restore-parsing` | 5 | 990 | one functional commit plus four local docs/spec commits | `candidate-rebuild` | [record](readiness-records/perf-element-map-restore-parsing.md) |
| `perf-bitmap-conversion-fast-path` | `perf/bitmap-conversion-fast-path` | 5 | 990 | one functional/test commit plus four local docs/spec commits | `candidate-rebuild` | [record](readiness-records/perf-bitmap-conversion-fast-path.md) |
| `16264-splashscreen-refactor` | `16264-splashscreen-refactor` | 6 | 990 | six functional commits; startup and GUI risk; no docs/spec contamination | `candidate-high-risk` | [record](readiness-records/16264-splashscreen-refactor.md) |

## Alias, Deferred, And Obsolete Routing

| Local ref | Current interpretation | Why it is not a canonical candidate |
|-----------|------------------------|-------------------------------------|
| `perf-included-file-restore-buffering` | alias/composite local branch | It bundles restore with a second `save` commit that `git cherry -v upstream/main` reports as already equivalent upstream; the slash-named restore branch is the cleaner source. |
| `perf/restore-baseline-support-docs` | support docs branch | Docs-only support branch referenced by several performance branches; not an upstream code PR candidate. |
| `16264-main-window-hidden-until-ready` | precursor branch | Its commits are already included in `16264-splashscreen-refactor`; use the latter record if the startup work continues. |
| `16264-main-window-show-after-restore` | separate later startup experiment | Not part of the current candidate set in this package revision. |

## Changed-Surface Snapshot

This package revision used `git diff --stat upstream/main...<branch>` only to
classify branch shape, not to claim rebase-ready final diffs.

| Canonical branch | Current touched surface summary |
|------------------|---------------------------------|
| `toolbar-persistence-save-settings` | `src/Gui/ToolBarManager.cpp`, `src/Gui/ToolBarManager.h` |
| `overlay-tab-destruction-cleanup` | `src/Gui/OverlayManager.cpp`, `src/Gui/OverlayWidgets.cpp`, `src/Gui/OverlayWidgets.h` |
| `perf/image-plane-lifecycle-lazy-texture` | `src/Gui/ViewProviderImagePlane.cpp`, `src/Gui/ViewProviderImagePlane.h`, plus unrelated local docs/spec files |
| `perf/tree-status-startup-followup` | `src/Gui/Tree.cpp`, `src/Gui/Tree.h`, plus unrelated local docs/spec files |
| `perf/included-file-restore-buffering` | `src/App/PropertyFile.cpp`, `src/Mod/Test/Document.py` |
| `perf/element-map-restore-parsing` | `src/App/ElementMap.cpp`, `tests/src/App/ElementMap.cpp`, plus unrelated local docs/spec files |
| `perf/bitmap-conversion-fast-path` | `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`, `tests/src/Gui/CMakeLists.txt`, plus unrelated local docs/spec files |
| `16264-splashscreen-refactor` | `src/Gui/Application.cpp`, `src/Gui/MainWindow.cpp`, `src/Gui/SplashScreen.*`, `src/Gui/StartupProcess.*`, `tests/src/Gui/SplashScreen.cpp`, `tests/src/Gui/CMakeLists.txt` |

## Ordering Guidance

The current low-to-high practical submission order remains:

1. `toolbar-persistence-save-settings`
2. `overlay-tab-destruction-cleanup`
3. `perf/included-file-restore-buffering`
4. `perf/element-map-restore-parsing`
5. `perf/tree-status-startup-followup`
6. `perf/image-plane-lifecycle-lazy-texture`
7. `perf/bitmap-conversion-fast-path`
8. `16264-splashscreen-refactor`

This is an execution heuristic, not a claim that every branch still deserves an
upstream PR after issue and overlap research.

## Research Method For The Next Slice

For the next active branch:

1. Reconfirm `upstream/main` has not moved, or refresh the package baseline.
2. Use the branch record as the single place to capture issue and PR overlap.
3. Decide whether the branch should be rebased directly or rebuilt from
   selected commits.
4. Write a branch-specific validation plan before any cleanup work starts.
5. Keep deferred or obsolete local refs out of the active path unless new
   evidence reactivates them.

## Open Questions

- Which active candidates still have suitable upstream issues with consensus?
- Does `perf/bitmap-conversion-fast-path` now overlap enough with current
  upstream image-conversion work to reduce its value as a standalone PR?
- Which GUI-visible candidates need Design Working Group awareness before any
  PR draft exists?
- Which candidates need non-Linux validation before they can be defended as
  ready?
