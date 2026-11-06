---
title: FreeCAD performance optimization tasks
doc_type: spec
artifact_type: tasks
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Tasks

## Context And Execution

Read requirements.md, design.md, traceability.md and verification.md before
implementation. Task IDs below are authoritative; historical checklists/plan
epics are migration inputs only. `[?]` needs a decision, `[/]` is partial,
`[ ]` is pending. Mark one selected implementation task `[~]` before coding.
Completion requires its acceptance and evidence; imported results stay dated.

## Task Dependency Graph

```text
T019, T021 -> T001
T019, T021, T018 -> T002
T019, T021, T012 -> T003
T001, T002, T003, T005, T006, T007, T020 -> T004
T019, T021, T015, T016 -> T005
T019, T021, T013 -> T006
T019, T021, T014, T017 -> T007
T004 -> T008
none -> T009
T002, T003, T004, T005, T006, T007 -> T010
T008, T009, T010, T021 -> T011
T019, T021 -> T012
T019, T021 -> T013
T019, T021 -> T014
T019, T021 -> T015
T015 -> T016
T019, T021, T014 -> T017
T019, T021 -> T018
T021 -> T019
T001, T003, T006, T007 -> T020
none -> T021
```

## Delivery Slices

Parent tasks are acceptance checkpoints. Execute bounded child tasks with the
linked requirements, design, decision ledger and validation manifest. Source
findings are static review evidence, not completed fixes. T021 completed the
decision-ledger documentation repair; D003/T019 must pin the candidate before
source-fix or acceptance work. D001/T001 approves performance pass rules before measurement.
Do not mark a source-safety finding complete merely because its spec was updated.

- [?] T001 Fix the current benchmark protocol and baseline revisions.
  - Depends on: T019, T021
  - Requirements: Requirement 3 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `open-decisions.md`, `verification.md`, `research.md`
  - Acceptance: D001 approves the candidate manifest, exact baseline/change revisions, fixture
    identities, per-optimization measurement inventory, primary metrics/endpoints, run
    count/statistic, warm-up/exclusion/cache policy, variability handling and
    improvement/non-regression thresholds before sampling. Include T020 latency/memory rules;
    removed switches are not assumed.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: reasoned
  - Evidence: May timings and transient scripts are historical; a comparable current protocol is
    absent.
  - Decision owner: local-developer
  - Open decisions: D001

- [/] T002 Verify included-file restore compatibility on the target revision.
  - Depends on: T019, T021, T018
  - Requirements: Requirement 1 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
    `tests/src/Base/Reader.cpp`
  - Acceptance: Requirement 1 AC1–AC3 pass on the pinned candidate: byte/ZIP/permission/notification
    checks plus T018 archive-level empty/nonempty failure matrix, partial destination disposition
    and later-entry continuation.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Buffered copy/error checks are present; imported Nov 4 tests and scratch roundtrips
    cover a subset, not all compatibility criteria.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-001

  - [ ] T002.1 Verify empty, binary, multi-buffer and multi-entry restored bytes and ZIP boundaries.
    - Depends on: T019, T021, T018
    - Requirements: Requirement 1 (all acceptance criteria)
    - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
      `tests/src/Base/Reader.cpp`
    - Acceptance: Verify empty, binary, multi-buffer and multi-entry restored bytes and ZIP
      boundaries.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T002.2 Verify property notifications, read-only destination behavior and read/write/close
    failures.
    - Depends on: T002.1
    - Requirements: Requirement 1 (all acceptance criteria)
    - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
      `tests/src/Base/Reader.cpp`
    - Acceptance: Verify read/write/close failures, notifications and read-only destinations with
      both property-hook checks and T018 document-level failure outcomes; a failing first entry must
      not hide the expected valid second-entry behavior.
    - Evidence mode: artifact
    - Evidence: Pending.

- [/] T003 Complete bitmap and rendered image correctness coverage.
  - Depends on: T019, T021, T012
  - Requirements: Requirement 2 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`
  - Acceptance: Requirement 2 AC1–AC3 pass: T012 extent/arithmetic guards, all direct/fallback
    formats, null/slender boundary outcomes and rendered orientation/color/alpha/scale/ownership are
    verified on the pinned revision.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Four direct-format tests exist; full fallback/rendered visual matrix remains open.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-002, CP-004

  - [ ] T003.1 Verify direct conversion for RGB32, ARGB32, RGBA8888 and RGBX8888.
    - Depends on: T019, T021, T012
    - Requirements: Requirement 2 (all acceptance criteria)
    - Files: `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`
    - Acceptance: Verify RGB32/ARGB32/RGBA8888/RGBX8888 pixel correctness and safe
      null/32767/32768/65537 slender dimension outcomes under T012; verify supported edge pixels and
      absence of invalid memory writes with sanitizer-equivalent evidence where available.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T003.2 Verify premultiplied, indexed, grayscale, high-depth and SVG-derived fallback
    results.
    - Depends on: T003.1
    - Requirements: Requirement 2 (all acceptance criteria)
    - Files: `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`
    - Acceptance: Verify premultiplied, indexed, grayscale, high-depth and SVG-derived fallback
      results.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T003.3 Record rendered-plane orientation, color, alpha, scale and texture ownership checks.
    - Depends on: T003.2
    - Requirements: Requirement 2 (all acceptance criteria)
    - Files: `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`
    - Acceptance: Record rendered-plane orientation, color, alpha, scale and texture ownership
      checks.
    - Evidence mode: artifact
    - Evidence: Pending.

- [ ] T004 Remeasure restore and image conversion comparably.
  - Depends on: T001, T002, T003, T005, T006, T007, T020
  - Requirements: Requirement 1 AC1–AC3; Requirement 2 AC1–AC3; Requirement 3 AC1–AC5; Requirement 4
    AC1–AC4; Requirement 5 AC1–AC4; Requirement 6 AC1–AC4
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `verification.md`, `research.md`
  - Acceptance: D001 decision rules pass for the pinned revision/fixture/build composition;
    per-change inventory covers retained restore/save, parser, image/lazy and tree patches or
    records reviewed exclusion/routing. T020 first-frame/first-show/repeat-show and material memory
    effects are included; deferred work and attribution limits are explicit.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Historical May restore and image improvements cannot establish current performance
    acceptance.
  - Open decisions: D001

  - [ ] T004.1 Record comparable restore/open benchmark runs under D001.
    - Depends on: T001, T002, T003, T005, T006, T007, T020
    - Requirements: Requirement 1, Requirement 2, Requirement 3 (all acceptance criteria)
    - Files: `verification.md`, `research.md`
    - Acceptance: Record comparable restore/open benchmark runs under D001.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T004.2 Record comparable image conversion/lifecycle timings and remaining bottlenecks under
    D001.
    - Depends on: T004.1, T020
    - Requirements: Requirement 1, Requirement 2, Requirement 3 (all acceptance criteria)
    - Files: `verification.md`, `research.md`
    - Acceptance: Record comparable conversion and T020 GUI
      first-frame/hidden-first-show/repeat-show latency, initial visibility, pending/materialized
      texture counts, process/decoded/Coin image memory and D001 non-regression decisions for direct
      and 1/2-component fallback fixtures.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T004.3 Record per-change parser and tree comparison or reviewed exclusions.
    - Depends on: T004.2, T005, T007
    - Requirements: Requirement 3 AC1–AC5; Requirement 4 AC1–AC4; Requirement 6 AC1–AC4
    - Design: design.md#evidence-applicability-and-benchmark-contract
    - Files: `verification.md`, `research.md`
    - Acceptance: Assign comparative baseline/change evidence to retained parser/tree optimizations
      and any buffered-save patch. Record per-change build/revision/fixture provenance and
      attribution limits; a combined document-open gain cannot silently stand in for each change.
      Any exclusion has a reviewed rationale and one owned residual destination.
    - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
      commands, results and limits in verification.md.
    - Evidence mode: artifact
    - Evidence: Pending D001 inventory and current comparative measurements.
    - Owner: local-developer
    - Open decisions: D001

- [/] T005 Verify element-map parser compatibility.
  - Depends on: T019, T021, T015, T016
  - Requirements: Requirement 4 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/App/ElementMap.cpp`, `tests/src/App/ElementMap.cpp`
  - Acceptance: Requirement 4 AC1–AC4 and CP-005 pass after T015/T016: bounded numeric
    grammar/legacy tolerance, semantic save–restore–save, bidirectional lookups after input-buffer
    release and dependent CAD subelement targets after reopen/recompute.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: string_view tokenizer and parser regression test inspected; Nov 4 focused result is
    imported evidence, not a fresh run.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-005

- [/] T006 Validate deferred full-resolution image-plane lifecycle.
  - Depends on: T019, T021, T013
  - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Gui/ViewProviderImagePlane.cpp`, `src/Gui/ViewProviderImagePlane.h`,
    `src/Gui/Document.cpp`, `verification.md`
  - Acceptance: Requirement 5 AC1–AC4 pass after T013: independently calibrated visible/hidden
    reopen, hidden import/paste then show, explicit edits, missing/external files and save/reopen
    retain stored XSize/YSize, placement and original resolution independently of temporary flags.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Pending texture, finishRestoring and show paths inspected; historical GUI smoke does
    not close the full matrix.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-002

  - [ ] T006.1 Verify visible-plane restore, edits and external/missing-file behavior.
    - Depends on: T019, T021, T013
    - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
    - Files: `src/Gui/ViewProviderImagePlane.cpp`, `verification.md`
    - Acceptance: Verify visible restored planes with independently non-default XSize/YSize,
      external/missing files and intentional image insertion/resize; calibration and placement must
      not be overwritten at finishRestoring.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T006.2 Verify hidden restore and show-after-restore preserve original texture resolution and
    plane dimensions.
    - Depends on: T006.1
    - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
    - Files: `src/Gui/ViewProviderImagePlane.cpp`, `verification.md`
    - Acceptance: Verify independently calibrated hidden reopen and hidden copy/paste/import
      followed by first show; compare dimensions, placement and original resolution before/after
      materialization and again after save/reopen.
    - Evidence mode: artifact
    - Evidence: Pending.

- [/] T007 Validate tree status batching and larger-model behavior.
  - Depends on: T019, T021, T014, T017
  - Requirements: Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Gui/Tree.cpp`, `src/Gui/Tree.h`, `verification.md`
  - Acceptance: Requirement 6 AC1–AC4 pass after T014/T017: receiver destruction/recreation and
    every suppressed open/direct-load/restore/import/paste/error path complete safely from idle
    timers without an unrelated edit/open; selection/focus/observers and per-object effects remain
    intact.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Single pending status update exists; per-object delete side effects are documented;
    larger-model coverage remains open.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-003

  - [ ] T007.1 Verify pending-status flush and open/close/reload, drag, expansion, selection and
    observer behavior.
    - Depends on: T019, T021, T014, T017
    - Requirements: Requirement 6 (all acceptance criteria)
    - Files: `src/Gui/Tree.cpp`, `src/Gui/Tree.h`, `verification.md`
    - Acceptance: Verify open/close/reload and idle direct doc.load/doc.restore/import/paste/error
      completion, eventual population/status/observer updates without another edit/open, and actual
      tree destruction/recreation followed by app signals. Preserve drag, expansion, selection and
      focus.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T007.2 Profile a larger representative document before proposing further batching or bulk
    deletion.
    - Depends on: T007.1
    - Requirements: Requirement 6 (all acceptance criteria)
    - Files: `src/Gui/Tree.cpp`, `verification.md`
    - Acceptance: Profile a larger representative document before proposing further batching or bulk
      deletion.
    - Evidence mode: artifact
    - Evidence: Pending.

- [?] T008 Decide whether GUI restore or startup profiling is warranted.
  - Depends on: T004
  - Requirements: Requirement 3 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `design.md`, `research.md`
  - Acceptance: Fresh timing either identifies one dominant substep with an owned follow-up or
    explicitly defers GUI restore/startup work; normal/clean profiles are distinguished.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: reasoned
  - Evidence: Historical plan C2/G3 remain conditional; no generic GUI/startup rewrite is
    authorized.
  - Decision owner: local-developer

- [?] T009 Disposition reduced-resolution texture exploration.
  - Depends on: none
  - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `design.md`, `docs/backlog/README.md`
  - Acceptance: D002 explicitly rejects or routes reduced-resolution work without representing
    current full-resolution lazy loading as its implementation.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: reasoned
  - Evidence: Reduced-resolution textures remain a separate behavior decision from implemented lazy
    loading.
  - Decision owner: local-developer
  - Open decisions: D002

- [ ] T010 Verify final production revisions and prepare review evidence.
  - Depends on: T002, T003, T004, T005, T006, T007
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `verification.md`, `docs/reference/document-open-performance.md`
  - Acceptance: D003 confirms exact final source/dependency revisions and platform scope; all
    affected automated, archive, calibration/lifecycle, visual/manual and benchmark evidence is
    rerun or has reviewed equivalence. Build/focused/full-suite results and per-optimization timings
    satisfy D001; no unresolved MoE acceptance blocker remains.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Focused reachable commits and dated results exist; exact PR revisions, full build/test
    and final review evidence remain unverified.
  - Open decisions: D003

  - [ ] T010.1 Confirm final production revisions and invalidate affected evidence.
    - Depends on: T002, T003, T004, T005, T006, T007
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Reconfirm the T019 manifest against final production patches; enumerate changed
      source/dependency/build/fixture/environment inputs and invalidate/reconcile affected
      correctness, manual/visual, archive, lifecycle and benchmark results before sign-off.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T010.2 Run pixi run build -j 2 and the affected focused suites on those revisions.
    - Depends on: T010.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Run pixi run build -j 2 and the affected focused suites on those revisions.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T010.3 Run pixi run test and record any failure/skip with its acceptance impact.
    - Depends on: T010.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Run pixi run test and record any failure/skip with its acceptance impact.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T010.4 Revalidate and assemble final performance, manual and platform evidence.
    - Depends on: T010.3
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Rerun affected measurements/manual checks or record reviewed equivalence tied to
      D003 final revisions; assemble per-optimization comparative results with D001 pass decisions,
      first-frame/show latency, memory effects, platform limits and focused commit boundaries. Old
      timing assembly alone is insufficient.
    - Evidence mode: artifact
    - Evidence: Pending.

- [ ] T011 Promote performance contracts and route residual work.
  - Depends on: T008, T009, T010, T021
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `docs/reference/document-open-performance.md`,
    `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
  - Acceptance: All original and added criteria, CP-004/CP-005 and F001–F009 have verified or
    explicitly reviewed/routed dispositions; no unresolved safety/acceptance blocker is hidden by
    task completion. Promote durable truth and record actual review/closure/archive metadata.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: No current closure or durable promotion claim; legacy plan/checklist remains
    historical.

  - [ ] T011.1 Promote accepted performance behavior/design and operational validation to durable
    targets.
    - Depends on: T008, T009, T010, T021
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `docs/reference/document-open-performance.md`,
      `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
    - Acceptance: Promote accepted performance behavior/design and operational validation to durable
      targets.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T011.2 Route conditional GUI/startup, reduced-resolution and other residual work to owned
    destinations.
    - Depends on: T011.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `docs/reference/document-open-performance.md`,
      `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
    - Acceptance: Route conditional GUI/startup, reduced-resolution and other residual work to owned
      destinations.
    - Evidence mode: artifact
    - Evidence: Pending.

  - [ ] T011.3 Verify acceptance/review coverage and record actual closure/archive metadata.
    - Depends on: T011.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `docs/reference/document-open-performance.md`,
      `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
    - Acceptance: Verify acceptance/review coverage and record actual closure/archive metadata.
    - Evidence mode: artifact
    - Evidence: Pending.

- [ ] T012 Guard bitmap dimensions and conversion arithmetic.
  - Depends on: T019, T021
  - Requirements: Requirement 2 AC2, AC3
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`
  - Acceptance: Reproduce the allocation/loop extent mismatch in a bounded regression; validate
    non-null/positive representable dimensions and checked offset/allocation arithmetic before Coin
    allocation. Define unsupported-input outcomes, preserve valid formats and test slender
    32767/32768/65537 boundaries plus null input; record sanitizer or equivalent memory-safety
    checks.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: F001 static source finding at c69560af8a: narrowed Coin allocation and original-size
    writes conflict; no runtime reproduction or fix yet.
  - Owner: local-developer
  - Properties: CP-004

- [ ] T013 Preserve calibrated image-plane dimensions during texture loading.
  - Depends on: T019, T021
  - Requirements: Requirement 5 AC2–AC4; Requirement 2 AC1
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/Gui/ViewProviderImagePlane.cpp`, `src/Gui/ViewProviderImagePlane.h`,
    `src/Gui/Document.cpp`, `verification.md`
  - Acceptance: Separate texture materialization from sizing saved/imported planes after
    restoring/import flags clear. Add calibrated X/Y and placement fixtures for visible/hidden
    reopen, hidden copy/paste/import then show, repeated show and save/reopen; retain intended
    new-image/explicit resize behavior.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: F002 independently confirmed by rendering and lifecycle reviewers through source
    ordering; runtime regression/fix remains pending.
  - Owner: local-developer
  - Properties: CP-002

- [ ] T014 Bind tree application callbacks to widget lifetime.
  - Depends on: T019, T021
  - Requirements: Requirement 6 AC3
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/Gui/Tree.cpp`, `src/Gui/Tree.h`, `verification.md`
  - Acceptance: Ensure both new start/finish-open subscriptions and queued work cease with
    TreeWidget lifetime using established scoped connections or explicit disconnection. Add actual
    destroy/recreate then application-open regression with stale receiver access detected; hiding a
    dock alone is insufficient.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: F003 source review found raw-this subscriptions omitted from explicit destructor
    disconnection; no runtime reproduction or fix yet.
  - Owner: local-developer
  - Properties: CP-003

- [ ] T015 Bound element-map numeric parsing and define compatibility.
  - Depends on: T019, T021
  - Requirements: Requirement 4 AC1, AC2
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/App/ElementMap.cpp`, `tests/src/App/ElementMap.cpp`
  - Acceptance: Document numeric conversion as part of the change; preserve approved legacy strtol
    tolerance or record an intentional compatibility decision. Test decimal/hex limits,
    LONG_MIN/LONG_MAX, overflow, signs, prefixes, whitespace, invalid suffixes and oversized tokens;
    no signed overflow, invalid access or uncontrolled partial acceptance.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: F004 source review found unchecked multiplication/addition in parseRestoreLong; no
    malformed-token runtime test or correction yet.
  - Owner: local-developer
  - Properties: CP-005

- [ ] T016 Qualify element-map and dependent CAD reference identity.
  - Depends on: T015
  - Requirements: Requirement 4 AC1, AC3, AC4
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/App/ElementMap.cpp`, `tests/src/App/ElementMap.cpp`, `verification.md`
  - Acceptance: Cover semicolon/colon/dollar-hashed forms, decimal child SIDs versus hexadecimal
    mapped SIDs above nine, multiple IDs/aliases and nested/shared children. Verify semantic
    save–restore–save, both lookup directions after input buffers die, and an existing CAD document
    with dependent subelement references through reopen/recompute; record fixture identities.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: F004 fixture review found current one-child parser regression does not establish all
    writer forms or dependent-feature identity.
  - Owner: local-developer
  - Properties: CP-005

- [ ] T017 Match tree restore suppression to all completion paths.
  - Depends on: T019, T021, T014
  - Requirements: Requirement 6 AC1, AC4
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/Gui/Tree.cpp`, `src/Gui/Tree.h`, `src/App/Document.cpp`, `src/Gui/Document.cpp`,
    `verification.md`
  - Acceptance: Define matching completion or safe rescheduling for app open, direct
    doc.load/doc.restore, import/copy-paste and failed restore. Narrow suppression to an owned
    operation or provide broader completion hooks based on source evidence. With timers initially
    idle, require tree population/status/observer updates without another edit/open and no stranded
    pending state on failure.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: F005 static review found isRestoring suppression broader than app finish-open
    flushing; user-visible starvation has not been reproduced.
  - Owner: local-developer
  - Properties: CP-003

- [ ] T018 Specify and verify document-level archive failure recovery.
  - Depends on: T019, T021
  - Requirements: Requirement 1 AC1–AC3
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
    `tests/src/Base/Reader.cpp`, `verification.md`
  - Acceptance: Record an approved outcome table for empty/nonempty read/write/close failures:
    diagnostics/failure lists, partial destination bytes/permissions, property notifications, and
    later-entry restoration. Exercise a failing first registered entry followed by a valid second at
    document level; preserve existing policy or explicitly approve/document a change rather than
    relying on a hook exception.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: F006 source review found XMLReader catches restore-hook exceptions; current fixture
    evidence does not specify all archive-level outcomes.
  - Owner: local-developer
  - Properties: CP-001

- [?] T019 Pin candidate validation revisions and evidence applicability.
  - Depends on: T021
  - Requirements: Requirement 3 AC1, AC2, AC5
  - Design: design.md#evidence-applicability-and-benchmark-contract
  - Files: `open-decisions.md`, `verification.md`, `research.md`
  - Acceptance: Resolve D003 initial candidate source/dependency commits, build/binary provenance,
    platform/toolchain/flags, fixture hashes and workflow before dependent acceptance. Define
    evidence invalidation after relevant changes and exact final confirmation at T010.1. Existing
    unpinned binaries remain historical.
  - Validation: Owner reviews and records candidate manifest and invalidation policy; verify every
    dependent task references the same manifest.
  - Evidence mode: artifact
  - Evidence: F008: final revisions were selected after acceptance tasks; candidate validation
    manifest is not yet approved.
  - Owner: local-developer
  - Open decisions: D003
  - Decision owner: local-developer

- [ ] T020 Measure deferred first-frame latency and image memory.
  - Depends on: T001, T003, T006, T007
  - Requirements: Requirement 3 AC3, AC4; Requirement 2 AC1; Requirement 5 AC1–AC4
  - Design: design.md#evidence-applicability-and-benchmark-contract
  - Files: `verification.md`, `research.md`, `src/Gui/ViewProviderImagePlane.cpp`,
    `src/Gui/BitmapFactory.cpp`
  - Acceptance: Under D001 pinned fixtures/endpoints, measure command restore, GUI
    open-to-first-correct-frame, hidden-first-show-to-frame and repeat-show. Record initial
    visibility, pending/materialized textures, peak process/decoded/Coin bytes for direct and
    1/2-component fallback formats; disposition material latency/memory regressions and separate
    shifted from eliminated work. Do not claim GPU residency from CPU bytes.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: F007: show performs deferred decode/conversion and fallback storage expands
    components; no comparative first-frame/memory measurements yet.
  - Owner: local-developer
  - Open decisions: D001

- [x] T021 Expose lifecycle decisions and verify execution guardrails.
  - Depends on: none
  - Requirements: Requirement 3 AC6
  - Design: design.md#open-questions
  - Files: `open-decisions.md`, `design.md`, `canonical-context.md`, `tasks.md`, `traceability.md`,
    `verification.md`
  - Acceptance: Use a repository-conforming decision ledger; spec_open_decisions must return
    unresolved D001/D002/D003 with accurate status and owner/blocking context. Verify task_context retains decision
    references, dependencies are satisfiable and no context-ready packet is treated as approval over
    review-needed decisions. Record actual tool checks before completing this documentation task.
  - Validation: Call spec_open_decisions and task_context for T001/T019; compare IDs, statuses,
    ownership and literal blockers; lint package and validate dependencies/links.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: MCP spec_open_decisions returned D001/D002/D003 as open with owners and
    blocking impacts; task_context(T021/T001/T019/T012) resolved traceability with zero gaps and
    retained D001/D003 references. lint_spec_package returned zero diagnostics. python3
    /tmp/check_spec003_moe.py passed 8 artifacts, 45 links, 6 requirements/24 ACs, 40 tasks,
    dependency DAG, F001-F009 destinations and historical preservation; root/nested-docs git diff
    --check passed. open-decisions.md and canonical-context.md explicitly prohibit interpreting
    advisory context readiness as execution approval. No source/runtime checks or decision
    resolutions claimed.
  - Owner: local-developer

  - Status: Documentation-only F009 repair verified; D001-D003 remain open and all source-safety
    findings remain pending.
## Legacy Task Disposition

The full 37-item checklist and May measurement tables are retained in
[legacy-tasks.md](legacy-tasks.md). Their original checkmarks are historical.
Plan epic IDs remain searchable in [plan.md](plan.md).

| Historical scope | Current task(s) | Reconciliation |
| --- | --- | --- |
| A1, B3, D1, E3; timing/review-note checkboxes | T001, T004, T010 | Dated measurements retained; current acceptance open |
| A2, B1, B2; buffered copy/notifications/fixtures | T002 | Implementation present; broader compatibility gates partial |
| C1; shape-map tracing and parser | T005 | Investigation history retained; parser exists; current compatibility evidence needed |
| C2; conditional GUI substep timing | T008 | Conditional on renewed measurements |
| D2, E1, E2; conversion/visual/format checks | T003 | Direct path exists; uncommon/visual matrix incomplete |
| F1, F2; restore ordering and lazy full-resolution prototype | T006 | Implemented; lifecycle matrix open |
| F2 reduced-resolution checkbox | T009 | Separate decision, not implemented |
| G1, G2; tree pending flag, side effects, larger trace | T007 | Batching exists; no bulk-delete bypass; larger-model evidence open |
| G3; startup tracing/ranking | T008 | Historical normal-profile observations; conditional follow-up |
| H1, H4; cleanup and commit separation | T010 | Reachable focused commits/absent switches are source evidence only |
| H2, H3; build, tests, PR gates | T010 | Current-revision final gates unverified |
| Promotion/closure not represented in legacy checklist | T011 | Required before closure |
