---
title: FreeCAD issues to consider
doc_type: reference
status: active
owner: local-developer
last_reviewed: 2026-05-15
---

# FreeCAD Issues to Consider

## Purpose

This reference captures a local triage snapshot of open FreeCAD issues that may
be reasonable candidates for contribution work.

## Source Of Truth

The canonical issue state is the upstream
[`FreeCAD/FreeCAD`](https://github.com/FreeCAD/FreeCAD/issues) issue tracker.
This document is only a point-in-time summary.

## Assumptions

Triage date: 2026-05-15

All issue details below were checked against `FreeCAD/FreeCAD` on GitHub during triage. Every issue in the original list was still open at the time of checking.

## Reference Data

### Recommended Priority

These are ordered from simplest likely quick fix to larger or riskier work. "Quick" is based on issue scope, labels, comments, and a light local code scan, not a full implementation pass.

| Priority | Issue | Why start here | Likely code area / notes |
| --- | --- | --- | --- |
| 1 | [#30009](https://github.com/FreeCAD/FreeCAD/issues/30009) Core/UI/UX: Selection View search does not reset when clearing text | Very small, confirmed bug, current behavior is clear. | `src/Gui/Selection/SelectionView.cpp`: `SelectionView::search()` only handles non-empty text; clearing should restore the unfiltered selection list and clear search state. |
| 2 | [#12735](https://github.com/FreeCAD/FreeCAD/issues/12735) Draft: `svgDiscretization` is obsolete | Contained cleanup. Active code reference appears limited. | `src/Mod/Draft/draftfunctions/svgshapes.py`; preference UI also has `svgDiscretization` in `src/Mod/Draft/Resources/ui/preferences-svg.ui`. Need decide whether to remove the preference or leave migration compatibility. |
| 3 | [#29409](https://github.com/FreeCAD/FreeCAD/issues/29409) Linux `StartupWMClass` is shared across versions | Small code footprint, but needs naming/version policy agreement. | `src/XDGData/org.freecad.FreeCAD.desktop` has `StartupWMClass=FreeCAD`; `src/Main/MainGui.cpp` sets `DesktopFileName`. |
| 4 | [#30047](https://github.com/FreeCAD/FreeCAD/issues/30047) FileOpen dialog confusing | Likely small UX fix if accepted: separate `.FCStd` and `.FCBak` filters. | Related to direct `.FCBak` opening added by #28454. Search file dialog/import filter registration. |
| 5 | [#30036](https://github.com/FreeCAD/FreeCAD/issues/30036) File -> Close closes active tab, not document | Menu/command location is clear; wording and behavior need care. | `src/Gui/Workbench.cpp` File menu currently uses `Std_CloseActiveWindow`; `src/Gui/CommandWindow.cpp` labels it `&Close`. Possible change: rename to "Close Tab" and add a separate "Close Document" command. |
| 6 | [#25128](https://github.com/FreeCAD/FreeCAD/issues/25128) Unit-system status bar button has variable width | Good first issue, likely UI sizing/text polish. | Need locate exact unit status bar widget. May involve stylesheet sizing or abbreviated label. |
| 7 | [#20757](https://github.com/FreeCAD/FreeCAD/issues/20757) Inconsistent naming of widgets in Status bar | Low technical risk, mostly naming consistency. | Needs UI wording decision. Suggested wording in issue: standardize on "Indicator". |
| 8 | [#9296](https://github.com/FreeCAD/FreeCAD/issues/9296) Tooltip HTML appears in status line | Likely compact central fix, but broad GUI surface. | Look at action/status-tip handling in `src/Gui/Command.cpp` and `src/Gui/Action.cpp`; sanitize rich text before sending to status line. Requires UI/UX review label. |
| 9 | [#16237](https://github.com/FreeCAD/FreeCAD/issues/16237) Macro tab shows full path in some open paths | Probably small, but issue is marked needs feedback. | Likely `src/Gui/EditorView.cpp` and macro/open-file paths. Reproduce drag/drop and Start tab behavior before editing. |
| 10 | [#15966](https://github.com/FreeCAD/FreeCAD/issues/15966) `FreeCAD.closeDocument(name)` fails with `-` in name | Testable and confirmed, but may touch Python command quoting or document naming behavior. | Search showed Python/C++ close paths in `src/App/ApplicationPy.cpp`, `src/App/Application.cpp`, and GUI command generation. Add regression test if fixed. |

### Other Good First Issues

These remain reasonable, but are less clearly "quick" than the top list because they need more design input, a bigger behavioral change, or deeper reproduction.

| Issue | Details / triage notes |
| --- | --- |
| [#23967](https://github.com/FreeCAD/FreeCAD/issues/23967) Gui: Add indicator for collapsed groups in Property View | Open, Good first issue. Needs Property View UI work and design consistency with recent collapse/expand behavior from #23535. |
| [#24067](https://github.com/FreeCAD/FreeCAD/issues/24067) Property with expression hides expression when selected | Open, Good first issue. User-facing property editor behavior; likely needs careful editor/delegate handling. |
| [#25340](https://github.com/FreeCAD/FreeCAD/issues/25340) Move Structure and Help toolbars to first row by default | Open, Good first issue, approved by UI/UX. Many comments; likely config/default layout work rather than logic. |
| [#17788](https://github.com/FreeCAD/FreeCAD/issues/17788) Docker build error, docs need update | Open, confirmed, docs/packaging. Need reproduce current Docker instructions before editing docs. |
| [#13346](https://github.com/FreeCAD/FreeCAD/issues/13346) Remove individual thumbnails from Start workbench Document tab | Open, Good first issue, has workaround. Requires Start workbench UI/context menu work. |
| [#13431](https://github.com/FreeCAD/FreeCAD/issues/13431) Referenced images cannot be updated after external change | Open, Good first issue. Likely needs reload action/refresh logic for image planes or referenced image objects. |
| [#13674](https://github.com/FreeCAD/FreeCAD/issues/13674) OpenSCAD import improvement for PartDesign WB | Open, Good first issue. Issue points at `src/Mod/OpenSCAD/importCSG.py`; behavior depends on current workbench and body creation/move flow. |
| [#13839](https://github.com/FreeCAD/FreeCAD/issues/13839) Measurement: display arrows between measured points | Open, Good first issue. Visual/interaction change; more involved than a small bug fix. |
| [#13897](https://github.com/FreeCAD/FreeCAD/issues/13897) Fit all and Fit selection use same icon with system icon theme | Open, Good first issue, Linux-specific. Needs icon theme/fallback investigation. |
| [#14520](https://github.com/FreeCAD/FreeCAD/issues/14520) Use Qt PDF library for PDF import | Open, Good first issue label, but likely non-trivial dependency/API migration. |
| [#15264](https://github.com/FreeCAD/FreeCAD/issues/15264) Addon Manager 3rd-party resources | Open, Good first issue, help wanted. Feature design spans Addon Manager resource types. |
| [#15978](https://github.com/FreeCAD/FreeCAD/issues/15978) Tree View settings are confusing and scattered | Open, Good first issue. Preferences reorganization; needs care for existing parameters and translations. |
| [#12842](https://github.com/FreeCAD/FreeCAD/issues/12842) Recent file list issues | Open, Good first issue, has workaround. Several subproblems; split into smaller PRs if attempted. |
| [#12705](https://github.com/FreeCAD/FreeCAD/issues/12705) File templates only available on start page | Open, Good first issue. Adds File menu entries for templates; needs command/menu design. |
| [#10408](https://github.com/FreeCAD/FreeCAD/issues/10408) Keyword arguments unavailable in Python commands | Open, Good first issue, docs/help wanted. First task may be documenting affected APIs before changing bindings. |
| [#8575](https://github.com/FreeCAD/FreeCAD/issues/8575) Remember workbench used when loading a document | Open, Good first issue. Cross-workbench workflow behavior; needs persistence format/compatibility decision. |
| [#6697](https://github.com/FreeCAD/FreeCAD/issues/6697) Store project backups in designated directory | Open, Good first issue. Touches backup policy and preferences; more than a one-line change. |
| [#5744](https://github.com/FreeCAD/FreeCAD/issues/5744) Fix IV mode in Sketchfab exporter | Open, confirmed, Good first issue. Old imported issue with limited details; needs reproduction/current exporter context. |

### Larger / Riskier Issues

These are likely higher impact, but they are not good quick-fix candidates.

| Issue | Details / triage notes |
| --- | --- |
| [#30113](https://github.com/FreeCAD/FreeCAD/issues/30113) Crash importing KiCAD board with KiCADStepUp addon | Open, confirmed regression, crash, milestone 1.2. Recent comments narrowed regression window around April 2026 builds and mention a possible related commit. High value, but needs crash reproduction and bisection/debugging. |
| [#29680](https://github.com/FreeCAD/FreeCAD/issues/29680) ProgressIndicator PR can freeze on Windows | Open, confirmed regression, solution proposed, milestone 1.2. Comment suggests a workaround around `BRepAlgoAPI_BooleanOperation::Build(/*progressRange*/)` in `src/Mod/Part/App/FCBRepAlgoAPI_BooleanOperation.cpp`. Requires Windows/OCC verification. |
| [#5566](https://github.com/FreeCAD/FreeCAD/issues/5566) Add progress indication for long tasks | Open, old feature request. Discussion points to OCCT progress APIs and prior ProgressIndicator work. Broad design/architecture task. |
| [#27171](https://github.com/FreeCAD/FreeCAD/issues/27171) GNOME "FreeCAD is not responding" dialog while opening files | Open, Linux, performance, has workaround. Comments suggest GNOME `check-alive-timeout` workaround and that async UI work may address it. |
| [#26808](https://github.com/FreeCAD/FreeCAD/issues/26808) Create standard performance benchmark | Open, performance/code quality feature. Useful infrastructure, but not a quick product fix. |
| [#5581](https://github.com/FreeCAD/FreeCAD/issues/5581) Multi-core/multithreaded support | Open, help wanted, assigned, performance/OCC. Very broad; comments discuss Sketcher, Assembly, OCCT, Eigen, and async UI. |
| [#18735](https://github.com/FreeCAD/FreeCAD/issues/18735) STEP import takes excessive time | Open, confirmed, help wanted, performance, OCC/STEP. Needs profiling and probably OCCT/import-path expertise. |
| [#18668](https://github.com/FreeCAD/FreeCAD/issues/18668) STEP importer does not support new Assembly module | Open, Assembly/STEP feature. Needs import model design, not a quick fix. |
| [#23250](https://github.com/FreeCAD/FreeCAD/issues/23250) Exception messages are not useful | Open, Good first issue but broad. Better approached by identifying one exception family first and adding object context there. |

## How To Update

Recheck issue state, labels, comments, and likely code areas before using this
list to choose work. Update `last_reviewed`, the triage date, and the priority
tables when the upstream issue tracker changes materially.

## Related Docs

- `docs/contribution-overview.md`
