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

Read requirements, design, baseline-status, traceability, verification and the
T019 validation manifest before acting. Task IDs remain stable while the file
is now arranged in the agreed delivery sequence. `Depends on` includes explicit
process sequencing as well as technical prerequisites; changing that order
requires a recorded replan, not selecting another ready task silently.

Completed T019/T021 provenance is retained. Current source presence and imported
historical checks live in [baseline-status.md](baseline-status.md), outside
execution-state accounting. All five remaining qualification checkpoints start
pending because none has a qualifying completed child/current run. Reserve `[/]`
for future work with verified completed portions of its own declared acceptance.
Never turn implementation presence, source findings or old passes into current
completion. A routed optional child is a scope/ownership disposition, not a
runtime pass; a parent may close only with its required runtime criteria and
that explicit routing verified.

## Execution Order

| Phase | Required sequence | Exit gate |
| --- | --- | --- |
| 1 — Planning | T021/T019 retained; T009 scope disposition; T001 complete | D001 protocol approved before source work |
| 2A — Images | T012/T013/T003/T006 complete | Safe conversion and calibrated lifecycle qualified |
| 2B — Tree | T014 → T017 → T007 | Receiver/completion fixes and current functional qualification |
| 2C — CAD maps | T015 → T016 → T005 | Bounded parsing and CAD reference identity qualified |
| 2D — Archives | T018 → T002 | Archive error outcomes and compatibility qualified |
| 3 — Performance | T020 → T004 | Predeclared latency/memory/per-change acceptance satisfied |
| 4 — Delivery | T010 → T011 | Exact final evidence, platform checks, promotion and closure |

Complete each fix and its focused regression before advancing, then close the
subgroup's broader qualification checkpoints. Use actual source/build/fixture
receipts under [validation-manifest.md](validation-manifest.md). No source work
or performance sampling was performed by this replan. Optional B009/B010/B011
stay in the backlog; all current MoE safety and acceptance obligations stay here.

## Task Dependency Graph

```text
none -> T021
T021 -> T019
none -> T009
T019, T021, T009 -> T001
T001, T019, T021 -> T012
T012 -> T013
T012, T013 -> T003
T003, T013 -> T006
T006 -> T014
T014 -> T017
T014, T017 -> T007
T007 -> T015
T015 -> T016
T015, T016 -> T005
T005 -> T018
T018 -> T002
T001, T002, T003, T005, T006, T007 -> T020
T001, T002, T003, T005, T006, T007, T020 -> T004
T004 -> T010
T010, T019, T021, T009 -> T011
none -> T008
```

## Phase 1 — Finish Planning

Exit satisfied: T021/T019 evidence retained, T009 scope disposition recorded, and T001
resolves D001 with the benchmark protocol. **Next action: T014 — Bind tree application callbacks to widget lifetime
(Phase 2B; image subgroup complete).**

- [x] T021 Expose lifecycle decisions and verify execution guardrails.
  - Depends on: none
  - Requirements: Requirement 3 AC6
  - Design: design.md#open-questions
  - Files: `open-decisions.md`, `design.md`, `canonical-context.md`, `tasks.md`, `traceability.md`,
    `verification.md`
  - Acceptance: Use a repository-conforming decision ledger; spec_open_decisions must return
    unresolved D001/D002/D003 with accurate status and owner/blocking context. Verify task_context
    retains decision references, dependencies are satisfiable and no context-ready packet is treated
    as approval over review-needed decisions. Record actual tool checks before completing this
    documentation task.
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
  - Status: Completed documentation/preparation scope; evidence remains dated. Current
    decision status is in open-decisions.md; no runtime result is implied.
  - Completion-time context: Documentation-only F009 repair verified; D001/D002 remain open; D003 initial candidate/policy is resolved by T019; source-safety findings remain pending.
- [x] T019 Pin candidate validation revisions and evidence applicability.
  - Depends on: T021
  - Requirements: Requirement 3 AC1, AC2, AC5
  - Design: design.md#evidence-applicability-and-benchmark-contract
  - Files: `validation-manifest.md`, `validation-manifest.json`, `open-decisions.md`,
    `verification.md`, `design.md`, `canonical-context.md`, `tasks.md`, `traceability.md`
  - Acceptance: Resolve D003 initial candidate source/dependency commits, build/binary provenance,
    platform/toolchain/flags, fixture hashes and workflow before dependent acceptance. Define
    evidence invalidation after relevant changes and exact final confirmation at T010.1. Existing
    unpinned binaries remain historical.
  - Validation: Owner reviews and records candidate manifest and invalidation policy; verify every
    dependent task references the same manifest.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: validation-manifest.md and validation-manifest.json
    (spec003-linux-x86_64-debug-candidate-20261106-v1) pin root
    f7fe2520c2edf25a17f3ad1fde084116cb1627d4/docs da79497b9b4ee447fbd8e5b0cde168af55b47118, seven
    matching clean recursive Git dependencies, locked Linux Debug profile, 510 package identities,
    43 checked file/tool hashes and nine fixture hashes. Eight existing binaries/libraries remain
    explicitly ineligible; four generated recipes remain unmaterialized. Direct metadata/hash
    checks, 38 explicit dependent-task bindings, acyclic task/link/history checks and
    root/nested-docs whitespace pass. MCP spec_open_decisions lists D001/D002 only;
    task_context(T019) has zero gaps/open blockers; package lint has zero diagnostics. D003 records
    delegated initial selection and evidence invalidation/final-confirmation policy. No application
    build/test, source correction or final PR/platform acceptance claimed.
  - Owner: local-developer
  - Resolved decisions: D003 (initial candidate/policy); final confirmation remains T010.1
  - Decision owner: local-developer
  - Status: Completed documentation/preparation scope; evidence remains dated. Current
    decision status is in open-decisions.md; no runtime result is implied.
  - Completion-time context: Initial candidate and D003 applicability policy complete under the user's T019 instruction. D001/D002, new build/runtime receipts, all source fixes and T010.1 final production confirmation remain pending.
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.

- [x] T009 Disposition reduced-resolution texture exploration.
  - Depends on: none
  - Requirements: Requirement 2 AC1; Requirement 5 AC2–AC4 (scope disposition only; no runtime
    criterion discharged)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `design.md`, `open-decisions.md`, `docs/backlog/README.md`,
    `docs/backlog/spec003-follow-ups.md`, `verification.md`
  - Acceptance: D002 explicitly rejects or routes reduced-resolution work without representing
    current full-resolution lazy loading as its implementation.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: 2026-11-06 user-authorized execution replan resolves D002 as scope/routing: retain original/full-resolution behavior in active spec 003; deferred B009 in docs/backlog/README.md and spec003-follow-ups.md has local-developer ownership, measured reactivation trigger, fidelity/calibration/resource boundaries and exit checks. No reduced-resolution prototype, visual result or Requirement 2/5 runtime acceptance is claimed.
  - Decision owner: local-developer
  - Resolved decisions: D002 (retain original-resolution behavior; optional proposal at B009)
  - Destination: docs/backlog/spec003-follow-ups.md#b009-reduced-resolution-texture-exploration

  - Status: Documentation scope/routing acceptance complete. B009 remains deferred; runtime safety/calibration/performance tasks remain pending.
- [x] T001 Fix the current benchmark protocol and baseline revisions.
  - Depends on: T019, T021, T009
  - Requirements: Requirement 3 AC1, AC3 (protocol definition); actual measurements remain Phase 3
  - Design: design.md#execution-phases-and-checkpoints;
    design.md#evidence-applicability-and-benchmark-contract
  - Files: `benchmark-protocol.md`, `benchmark-plan.json`, `benchmark-runner.py`,
    `open-decisions.md`, `verification.md`; immutable `validation-manifest.md` is an input
  - Acceptance: D001 approves the candidate manifest, exact baseline/change revisions, fixture
    identities, per-optimization measurement inventory, primary metrics/endpoints, run
    count/statistic, warm-up/exclusion/cache policy, variability handling and
    improvement/non-regression thresholds before sampling. Include T020 latency/memory rules;
    removed switches are not assumed. Record one runnable bounded protocol artifact, exact
    fixture/revision scope, predeclared improvement/non-regression rules and owner decision for
    D001. This task produces the planning decision; it does not certify runtime results. Phase 2
    cannot start until this task is complete.
  - Validation: Review the protocol against Requirement 3 and T019 applicability; verify D001
    resolution, all four phase gates and baseline/changed composition choices before source work.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: benchmark-protocol.md, benchmark-plan.json and benchmark-runner.py predeclare eight exact source-origin pairs, hashed fixture scope, matched optimized arms, 20 measured pairs, endpoints/statistics/variability and timing/latency/resource gates. Collector --check --self-test and --schedule passed; synthetic timeout/output-cap/failure handling verified. Direct checker passed 40 stable task IDs, 79 local links, completed provenance and immutable/history preservation, eight public fixtures and private document/three bounded PNG identities; source/config unchanged. MCP decisions returns none; task_context(T001) has zero gaps/open decisions; package lint zero diagnostics; root/nested docs whitespace passes. D001 records delegated owner selection. Corrected runtime arms, generated outputs, probes and actual samples remain Phase 3 after Phase 2 qualification.
  - Decision owner: local-developer
  - Resolved decisions: D001
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: May timings and transient scripts are historical; a comparable current
    protocol is now predeclared in benchmark-protocol.md; runtime results remain pending.
  - Status: Completed Phase 1 protocol definition and delegated D001 selection. Next T012; no source correction or runtime/performance acceptance claimed.

## Phase 2 — Complete Correction And Qualification Slices

Each subgroup has one coherent outcome. Finish its fix-focused regressions and qualification before
entering the next subgroup. Source or environment changes carry actual execution receipts and
invalidate affected earlier evidence.

### 2A — Image Safety, Calibration And Fidelity

Exit: T012/T013 corrections and T003/T006 required local acceptance pass on a named composition.

- [x] T012 Guard bitmap dimensions and conversion arithmetic.
  - Depends on: T001, T019, T021
  - Requirements: Requirement 2 AC2, AC3
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/Gui/BitmapFactory.cpp`, `src/Gui/BitmapFactory.h`,
    `tests/src/Gui/BitmapFactory.cpp`, `t012-validation.json`, `verification.md`
  - Acceptance: Reproduce the allocation/loop extent mismatch in a bounded regression; validate
    non-null/positive representable dimensions and checked offset/allocation arithmetic before Coin
    allocation. Define unsupported-input outcomes, preserve valid formats and test slender
    32767/32768/65537 boundaries plus null input; record sanitizer or equivalent memory-safety
    checks.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: QImage-to-Coin conversion now validates null/positive signed-short extents and RGBA byte capacity before narrowing/allocation, uses size_t direct row offsets, and clears invalid/failed-fallback output; public signature unchanged and header documents failure. Bounded old-code regression returned 1x1 for 65537x1 fallback; old direct path Memcheck reproduced an invalid write past a 4-byte Coin allocation (exit 99). Fresh pixi run --locked build -j 2 passed; final executable passed all 30 focused tests and all 30 under Memcheck with zero errors (one narrowly scoped libunwind startup probe suppression; leak/undefined-value checks disabled). Focused pre-commit and CRLF-aware whitespace pass. t012-validation.json pins actual root + three-file diff/source hashes, 7 matching clean dependencies, 510 packages, config/tool/build/binary/runtime/fixture/log identities and limitations. Direct checker passed 40 stable task IDs, 81 links/anchors, frozen manifests/history/protocol, completed evidence and unrelated console preservation; package lint zero diagnostics and T012 context zero gaps. No native viewport, 32-bit, broader calibration/lifecycle, performance or final PR acceptance claimed.
  - Owner: local-developer
  - Properties: CP-004
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F001 reproduced on the old converter and corrected with local
    boundary/CPU memory-access qualification; see verification.md and t012-validation.json.
    Broader T003 image-format/visual qualification remains pending.

  - Status: Completed local extent/arithmetic correction and CPU memory-access qualification. Next T013; broader T003/T006/native GUI/performance/platform acceptance remains pending.
- [x] T013 Preserve calibrated image-plane dimensions during texture loading.
  - Depends on: T012
  - Requirements: Requirement 5 AC2–AC4; Requirement 2 AC1
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/Gui/ViewProviderImagePlane.cpp`, `src/Gui/ViewProviderImagePlane.h`,
    `tests/src/Gui/ImagePlane.cpp`, `tests/src/Gui/CMakeLists.txt`,
    `t013-validation.json`, `verification.md`; `src/Gui/Document.cpp` inspected unchanged
  - Acceptance: Separate texture materialization from sizing saved/imported planes after
    restoring/import flags clear. Add calibrated X/Y and placement fixtures for visible/hidden
    reopen, hidden copy/paste/import then show, repeated show and save/reopen; retain intended
    new-image/explicit resize behavior.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: private Initialize/Preserve sizing policy separates fresh/replaced image sizing from pending texture/DPI materialization; both restore and ObjImporting defer loads. Gui::Document ordering remains unchanged. Bounded old-provider run failed four rows: hidden reopen/flags-cleared visible finish reset XSize 123.25 to 20 and geometry; hidden copy/merge imported too early. Corrected pixi run --locked build -j 2 passed; ImagePlane_Tests_run passed seven functional rows plus init/cleanup (9 passes, no skips/failures); registered CTest passed with 120s timeout; all 30 retained bitmap regressions passed. Real App/Gui FCStd restore/copy/import fixtures assert independent X/Y calibration, Placement, Coin translation/quad geometry, pending/full 40x20 texture and DPI, repeated show/save-reopen and fresh/replaced raster/explicit SVG sizing. t013-validation.json records actual root + source/new-test patch hashes, 7 matching clean dependencies, 510 package identities, config/tool/build/binary/runtime/fixture/log provenance and limits. Pre-commit and CRLF-aware whitespace pass; package lint zero diagnostics, task context zero gaps; direct checker validates 40 stable IDs, prior completed/frozen artifacts, 83 links/anchors and unrelated console preservation. No native frame/visual matrix, cross-platform, sanitizer or performance acceptance claimed.
  - Owner: local-developer
  - Properties: CP-002
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F002 reproduced on the old provider; explicit pending-load preservation
    and importing deferral pass the local calibrated archive/copy/import/resize fixtures.
    See verification.md and t013-validation.json; broader T003/T006 acceptance remains pending.

  - Status: Completed local calibration-preserving deferred materialization/import correction and declared lifecycle fixtures. Next T003; broader T006/native-view/format/performance/platform acceptance remains pending.
- [x] T003 Complete bitmap and rendered image correctness coverage.
  - Depends on: T012, T013
  - Requirements: Requirement 2 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `tests/src/Gui/BitmapFactory.cpp`, `tests/src/Gui/ImagePlaneRender.cpp`,
    `tests/src/Gui/CMakeLists.txt`, `t003-validation.json`, `t003-render-contact.png`,
    `verification.md`; production converter/provider/renderer inspected unchanged
  - Acceptance: Requirement 2 AC1–AC3 pass: T012 extent/arithmetic guards, all direct/fallback
    formats, null/slender boundary outcomes and rendered orientation/color/alpha/scale/ownership are
    verified on the pinned revision.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: T003.1/T003.2/T003.3 completed in dependency order. Fresh pixi run --locked build -j 2 passes; 82 direct/fallback/boundary/ownership cases pass normally and under Memcheck with zero errors (one exact libunwind startup-probe suppression; leak/undefined-value checks disabled). All 36 valid Qt 6.11.3 formats plus indexed color/gray alpha, high-depth non-byte-aligned data, padded external storage and SVG are covered; direct scalar checks are exact, normalized fallback bound is one RGBA8 level. Real provider scenes through production SoQtOffscreenRenderer pass 39 format rows plus analytic calibrated scale/placement in both native X11/xcb and Qt offscreen Mesa llvmpipe contexts (42 QtTest passes each, no skips). Full-pixel scalar parity and independent orientation/color/source-over alpha/geometry/texture-lifetime assertions pass under predeclared limits; all 41 PNG hashes match repeated/offscreen/native profiles. Two CTest targets (render and existing calibration/archive suite) and pre-commit/whitespace pass. t003-validation.json binds actual source/test/new-file diff, 7 clean dependencies, 510 packages, build/runtime/context/log/output identities; t003-render-contact.png retained and visually inspected by Codex. Direct checker passes 40 stable tasks, 6 prior completed records, 87 links/anchors and frozen/unrelated preservation; package lint and child contexts have zero diagnostics/gaps. Production source unchanged. No interactive viewer, hardware GPU, scanout, performance/resource budget or final-platform acceptance claimed.
  - Status: Completed current local Requirement 2 AC1–AC3 qualification and all three children. Next T006; interactive/native-frame timing, broad lifecycle, platform and final PR gates remain pending.
  - Properties: CP-002, CP-004
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Baseline: baseline-status.md#bitmap-conversion

  - [x] T003.1 Verify direct conversion for RGB32, ARGB32, RGBA8888 and RGBX8888.
    - Depends on: T012, T013
    - Requirements: Requirement 2 (all acceptance criteria)
    - Files: `tests/src/Gui/BitmapFactory.cpp`, `t003-validation.json`, `verification.md`
    - Acceptance: Verify RGB32/ARGB32/RGBA8888/RGBX8888 pixel correctness and safe
      null/32767/32768/65537 slender dimension outcomes under T012; verify supported edge pixels and
      absence of invalid memory writes with sanitizer-equivalent evidence where available.
    - Evidence mode: artifact
    - Evidence: 2026-11-06: all four direct formats retain exact byte/component and scalar-reference results; expanded width/height 32767/32768/65537 fixtures, null and replacement-output cases pass. Current 82-case bitmap suite passes normally and under Memcheck with zero errors (only the narrow libunwind startup-probe suppression; leak/undefined-value checks disabled). Fresh locked Pixi build passes; t003-validation.json pins current source/test/binary/context/log identities. No unsupported extent is written; T012 production guard is unchanged.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - Status: Completed current local qualification; scope and final-applicability limits are in t003-validation.json.
  - [x] T003.2 Verify premultiplied, indexed, grayscale, high-depth and SVG-derived fallback results.
    - Depends on: T003.1
    - Requirements: Requirement 2 (all acceptance criteria)
    - Files: `tests/src/Gui/BitmapFactory.cpp`, `t003-validation.json`, `verification.md`
    - Acceptance: Verify premultiplied, indexed, grayscale, high-depth and SVG-derived fallback
      results.
    - Evidence mode: artifact
    - Evidence: 2026-11-06: all 36 valid Qt 6.11.3 formats (32 fallback), indexed color/gray alpha palettes, non-byte-aligned RGBA64, odd/padded/external strides and SVG-derived pixels pass independent QColor scalar reference checks. Canonical fallback quantization bound is one RGBA8 level, predeclared; direct comparisons are exact. Source mutation/destruction, backing-buffer death and Coin copy/original-clear ownership assertions pass in the current 82-case normal/Memcheck suite. Native-X11 and offscreen render matrices also pass. See t003-validation.json; no production conversion changes were needed.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - Status: Completed current local qualification; scope and final-applicability limits are in t003-validation.json.
  - [x] T003.3 Record rendered-plane orientation, color, alpha, scale and texture ownership checks.
    - Depends on: T003.2
    - Requirements: Requirement 2 (all acceptance criteria)
    - Files: `tests/src/Gui/ImagePlaneRender.cpp`, `tests/src/Gui/CMakeLists.txt`,
      `t003-validation.json`, `t003-render-contact.png`, `verification.md`
    - Acceptance: Record rendered-plane orientation, color, alpha, scale and texture ownership
      checks.
    - Evidence mode: artifact
    - Evidence: 2026-11-06: real image-plane provider scenes passed 39 format rows plus calibrated scale/placement in both native X11/xcb and Qt offscreen software-GL contexts (42 QtTest passes each including setup/cleanup, zero skips). Production SoQtOffscreenRenderer compared every pixel against an independent scalar luminance/LA/RGB/RGBA render with fixed 2-level bound; texel-center orientation/color/source-over alpha uses a fixed 3-level bound; exact world-scale and translated/rotated bounding rectangles pass. Texture data remains correct after caller QImage and temporary Coin image destruction. All 41 PNG hashes match across repeated/offscreen/native runs; retained t003-render-contact.png was visually inspected by Codex. Current build, two CTest targets and pre-commit pass. t003-validation.json records renderer/source/build/output/log scope; no interactive viewer/frame timing, hardware-GPU or performance claim.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - Status: Completed current local qualification; scope and final-applicability limits are in t003-validation.json.
- [x] T006 Validate deferred full-resolution image-plane lifecycle.
  - Depends on: T003, T013
  - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Gui/ViewProviderImagePlane.cpp`, `src/Gui/ViewProviderImagePlane.h`,
    `tests/src/Gui/ImagePlane.cpp`, `t006-validation.json`, `verification.md`
  - Acceptance: Requirement 5 AC1–AC4 pass after T013: independently calibrated visible/hidden
    reopen, hidden import/paste then show, explicit edits, missing/external files and save/reopen
    retain stored XSize/YSize, placement and original resolution independently of temporary flags.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: T006.1 and T006.2 completed in dependency order. t006-validation.json and verification.md#t006-deferred-image-plane-lifecycle bind actual root f46d1d28e7/docs 946e262 plus exact three-file tracked diff, build/runtime/source/log hashes, 7 clean dependencies and 510 matching packages. Final old-provider reproduction 16 passes/8 failures; early-show deferral, failed/empty stale texture and unsupported-image calibration faults corrected. Full pixi build, 24 lifecycle Qt passes (22 functional rows), 82 bitmap cases, 42 native X11 software-render Qt passes, 2 CTest targets and scoped pre-commit pass; zero acceptance test skips/failures. All 41 current render PNG hashes match immutable T003 outputs. Direct checker passes exact hashes, 40 stable tasks, 10 prior completed task records, 88 links/anchors, frozen artifacts/receipts, CRLF and unrelated console preservation. Package lint zero diagnostics. Lifecycle Memcheck attempts exit 99 before tests in Qt/IBus startup; explicitly unqualified and routed to T010. Missing archive-entry save/OriginalName metadata audit remains T018. No performance, interactive-frame, hardware-GPU or final platform acceptance claimed.
  - Status: Completed current local Requirement 5 AC1–AC4 and requalified Requirement 2 regressions; Phase 2A complete. Next T014; final/runtime/platform and performance gates remain T010/T020/T004.
  - Properties: CP-002
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Baseline: baseline-status.md#image-plane-lifecycle

  - [x] T006.1 Verify visible-plane restore, edits and external/missing-file behavior.
    - Depends on: T003, T013
    - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
    - Files: `src/Gui/ViewProviderImagePlane.cpp`, `src/Gui/ViewProviderImagePlane.h`,
      `tests/src/Gui/ImagePlane.cpp`, `t006-validation.json`, `verification.md`
    - Acceptance: Verify visible restored planes with independently non-default XSize/YSize,
      external/missing files and intentional image insertion/resize; calibration and placement must
      not be overwritten at finishRestoring.
    - Status: Completed local visible/edit/error lifecycle qualification; archive metadata/error policy remains T018 and final runtime/platform applicability T010.
    - Evidence mode: artifact
    - Evidence: 2026-11-06: t006-validation.json and verification.md#t006-deferred-image-plane-lifecycle record visible PNG/SVG calibrated reopen, early-show restore/import ready/late payloads, explicit replacement/resize, removed/corrupt/unsupported/empty-property behavior, invalid external assignment and embedded-payload survival after external deletion. Final red run 16 passes/8 failures; corrected provider 24 Qt passes (22 functional rows), zero skips. Calibration, Placement, geometry, original resolution and DPI assertions pass; build, current 82 bitmap regressions, 42 native render passes, two CTest targets and scoped pre-commit pass. Lifecycle Memcheck attempts stop before tests in Qt/IBus; no memory-clean claim; T010 owns revalidation.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - [x] T006.2 Verify hidden restore and show-after-restore preserve original texture resolution and plane dimensions.
    - Depends on: T006.1
    - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
    - Files: `src/Gui/ViewProviderImagePlane.cpp`, `tests/src/Gui/ImagePlane.cpp`,
      `t006-validation.json`, `verification.md`
    - Acceptance: Verify independently calibrated hidden reopen and hidden copy/paste/import
      followed by first show; compare dimensions, placement and original resolution before/after
      materialization and again after save/reopen.
    - Status: Completed local hidden pending/materialized lifecycle qualification; T006.1 prerequisites satisfied; final profile applicability remains T010.
    - Evidence mode: artifact
    - Evidence: 2026-11-06: t006-validation.json and verification.md#t006-deferred-image-plane-lifecycle bind hidden PNG/SVG reopen/first show/save-reopen, calibrated copy/paste/import after source closure, hidden resize/save before first show and repeated show without implicit reload. All 22 functional lifecycle rows pass (24 Qt passes), with original texture resolution, independently stored dimensions, Placement, geometry and DPI assertions. Two CTest targets pass; current native render outputs match all 41 immutable T003 PNG hashes.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

### 2B — Tree Lifetime And Restore Completion

Exit: T014/T017 corrections and T007.1 current tree qualification pass; T007.2 has owned B011
routing, with no new optimization claim.

- [ ] T014 Bind tree application callbacks to widget lifetime.
  - Depends on: T006
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
  - Evidence: Pending.
  - Owner: local-developer
  - Properties: CP-003
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F003 source review found raw-this subscriptions omitted from explicit
    destructor disconnection; no runtime reproduction or fix yet.

- [ ] T017 Match tree restore suppression to all completion paths.
  - Depends on: T014
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
  - Evidence: Pending.
  - Owner: local-developer
  - Properties: CP-003
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F005 static review found isRestoring suppression broader than app finish-open
    flushing; user-visible starvation has not been reproduced.

- [ ] T007 Validate tree status batching and larger-model behavior.
  - Depends on: T014, T017
  - Requirements: Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Gui/Tree.cpp`, `src/Gui/Tree.h`, `verification.md`
  - Acceptance: Requirement 6 AC1–AC4 pass after T014/T017 on representative small and larger pinned
    documents: receiver destruction/recreation and every suppressed
    open/direct-load/restore/import/paste/error path complete safely from idle timers without
    unrelated edit/open; selection/focus/observers and per-object effects remain intact. T007.2 is
    an explicitly routed future-optimization outcome at B011, not a required current runtime check;
    retained tree comparative evidence remains T004.3.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Pending current qualification. Replanning retained prior implementation and dated test observations in baseline-status.md; no child/current expanded acceptance result is complete.
  - Status: Remaining execution is pending; source presence/historical results are baseline context, not partial execution.
  - Properties: CP-003
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Baseline: baseline-status.md#tree-status-batching
  - Required subtasks: T007.1
  - Routed subtasks: T007.2 at B011; verify owned routing, not future profile execution.

  - [ ] T007.1 Verify pending-status flush and open/close/reload, drag, expansion, selection and observer behavior.
    - Depends on: T014, T017
    - Requirements: Requirement 6 (all acceptance criteria)
    - Files: `src/Gui/Tree.cpp`, `src/Gui/Tree.h`, `verification.md`
    - Acceptance: Verify open/close/reload and idle direct doc.load/doc.restore/import/paste/error
      completion, eventual population/status/observer updates without another edit/open, and actual
      tree destruction/recreation followed by app signals. Preserve drag, expansion, selection and
      focus. Include small and larger pinned documents for current functional/observer correctness;
      later optimization profiling is separate B011 work.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

The conditional T007.2 investigation is preserved under Routed Follow-Ups;
it is excluded from this parent's current runtime completion gate.

### 2C — Numeric Parsing And CAD Identity

Exit: bounded conversion, writer/alias/child identity and dependent CAD target qualification are
evidenced before T005 sign-off.

- [ ] T015 Bound element-map numeric parsing and define compatibility.
  - Depends on: T007
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
  - Evidence: Pending.
  - Owner: local-developer
  - Properties: CP-005
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F004 source review found unchecked multiplication/addition in
    parseRestoreLong; no malformed-token runtime test or correction yet.

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
  - Evidence: Pending.
  - Owner: local-developer
  - Properties: CP-005
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F004 fixture review found current one-child parser regression does not
    establish all writer forms or dependent-feature identity.

- [ ] T005 Verify element-map parser compatibility.
  - Depends on: T015, T016
  - Requirements: Requirement 4 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/App/ElementMap.cpp`, `tests/src/App/ElementMap.cpp`
  - Acceptance: Requirement 4 AC1–AC4 and CP-005 pass after T015/T016: bounded numeric
    grammar/legacy tolerance, semantic save–restore–save, bidirectional lookups after input-buffer
    release and dependent CAD subelement targets after reopen/recompute.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Pending current qualification. Replanning retained prior implementation and dated test observations in baseline-status.md; no child/current expanded acceptance result is complete.
  - Status: Remaining execution is pending; source presence/historical results are baseline context, not partial execution.
  - Properties: CP-005
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Baseline: baseline-status.md#element-map-parsing

### 2D — Archive Failure Recovery And Compatibility

Exit: document-level failure outcomes and included-file compatibility pass before comparative
measurements.

- [ ] T018 Specify and verify document-level archive failure recovery.
  - Depends on: T005
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
  - Evidence: Pending.
  - Owner: local-developer
  - Properties: CP-001
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F006 source review found XMLReader catches restore-hook exceptions; current
    fixture evidence does not specify all archive-level outcomes. T006 additionally observed
    PropertyFileIncluded::setValue writes _OriginalName before validating existence; a rejected
    external assignment retains getValue/texture/calibration, but metadata transactionality is
    unqualified. Include that metadata audit and missing-payload save outcomes here; owner
    local-developer. This is source evidence, not an approved policy change.

- [ ] T002 Verify included-file restore compatibility on the target revision.
  - Depends on: T018
  - Requirements: Requirement 1 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
    `tests/src/Base/Reader.cpp`
  - Acceptance: Requirement 1 AC1–AC3 pass on the pinned candidate: byte/ZIP/permission/notification
    checks plus T018 archive-level empty/nonempty failure matrix, partial destination disposition
    and later-entry continuation.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Pending current qualification. Replanning retained prior implementation and dated test observations in baseline-status.md; no child/current expanded acceptance result is complete.
  - Status: Remaining execution is pending; source presence/historical results are baseline context, not partial execution.
  - Properties: CP-001
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Baseline: baseline-status.md#included-file-restore

  - [ ] T002.1 Verify empty, binary, multi-buffer and multi-entry restored bytes and ZIP boundaries.
    - Depends on: T018
    - Requirements: Requirement 1 (all acceptance criteria)
    - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
      `tests/src/Base/Reader.cpp`
    - Acceptance: Verify empty, binary, multi-buffer and multi-entry restored bytes and ZIP
      boundaries.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - [ ] T002.2 Verify property notifications, read-only destination behavior and read/write/close failures.
    - Depends on: T002.1
    - Requirements: Requirement 1 (all acceptance criteria)
    - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
      `tests/src/Base/Reader.cpp`
    - Acceptance: Verify read/write/close failures, notifications and read-only destinations with
      both property-hook checks and T018 document-level failure outcomes; a failing first entry must
      not hide the expected valid second-entry behavior.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

## Phase 3 — Qualify Comparative Performance

Exit: T020/T004 satisfy the predeclared D001 endpoint, latency, memory, per-change and
non-regression contract. Earlier source-fix tests do not prove speedup.

- [ ] T020 Measure deferred first-frame latency and image memory.
  - Depends on: T001, T002, T003, T005, T006, T007
  - Requirements: Requirement 3 AC3, AC4; Requirement 2 AC1; Requirement 5 AC1–AC4
  - Design: design.md#evidence-applicability-and-benchmark-contract
  - Files: `verification.md`, `benchmark-protocol.md`, `performance-results.md`, `performance-results.json`, `src/Gui/ViewProviderImagePlane.cpp`,
    `src/Gui/BitmapFactory.cpp`
  - Acceptance: Under D001 pinned fixtures/endpoints, measure command restore, GUI
    open-to-first-correct-frame, hidden-first-show-to-frame and repeat-show. Record initial
    visibility, pending/materialized textures, peak process/decoded/Coin bytes for direct and
    1/2-component fallback formats; disposition material latency/memory regressions and separate
    shifted from eliminated work. Do not claim GPU residency from CPU bytes.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer
  - Resolved decisions: D001
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F007: show performs deferred decode/conversion and fallback storage expands
    components; no comparative first-frame/memory measurements yet.

- [ ] T004 Remeasure restore and image conversion comparably.
  - Depends on: T001, T002, T003, T005, T006, T007, T020
  - Requirements: Requirement 1 AC1–AC3; Requirement 2 AC1–AC3; Requirement 3 AC1–AC5; Requirement 4
    AC1–AC4; Requirement 5 AC1–AC4; Requirement 6 AC1–AC4
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `verification.md`, `benchmark-protocol.md`, `performance-results.md`, `performance-results.json`
  - Acceptance: D001 decision rules pass for the pinned revision/fixture/build composition;
    per-change inventory covers retained restore/save, parser, image/lazy and tree patches or
    records reviewed exclusion/routing. T020 first-frame/first-show/repeat-show and material memory
    effects are included; deferred work and attribution limits are explicit.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Pending.
  - Resolved decisions: D001
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: Historical May restore and image improvements cannot establish current
    performance acceptance.

  - [ ] T004.1 Record comparable restore/open benchmark runs under D001.
    - Depends on: T001, T002, T003, T005, T006, T007, T020
    - Requirements: Requirement 1, Requirement 2, Requirement 3 (all acceptance criteria)
    - Files: `verification.md`, `benchmark-protocol.md`, `performance-results.md`, `performance-results.json`
    - Acceptance: Record comparable restore/open benchmark runs under D001.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - [ ] T004.2 Record comparable image conversion/lifecycle timings and remaining bottlenecks under D001.
    - Depends on: T004.1, T020
    - Requirements: Requirement 1, Requirement 2, Requirement 3 (all acceptance criteria)
    - Files: `verification.md`, `benchmark-protocol.md`, `performance-results.md`, `performance-results.json`
    - Acceptance: Record comparable conversion and T020 GUI
      first-frame/hidden-first-show/repeat-show latency, initial visibility, pending/materialized
      texture counts, process/decoded/Coin image memory and D001 non-regression decisions for direct
      and 1/2-component fallback fixtures.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - [ ] T004.3 Record per-change parser and tree comparison or reviewed exclusions.
    - Depends on: T004.2, T005, T007
    - Requirements: Requirement 3 AC1–AC5; Requirement 4 AC1–AC4; Requirement 6 AC1–AC4
    - Design: design.md#evidence-applicability-and-benchmark-contract
    - Files: `verification.md`, `benchmark-protocol.md`, `performance-results.md`, `performance-results.json`
    - Acceptance: Assign comparative baseline/change evidence to retained parser/tree optimizations
      and any buffered-save patch. Record per-change build/revision/fixture provenance and
      attribution limits; a combined document-open gain cannot silently stand in for each change.
      Any exclusion has a reviewed rationale and one owned residual destination.
    - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
      commands, results and limits in verification.md.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Owner: local-developer
    - Resolved decisions: D001
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.
    - Context evidence: D001 inventory is predeclared; current comparative measurements remain pending.

## Phase 4 — Prepare Delivery And Close

Exit: exact final-production evidence applicability, required broad/platform checks, durable
promotion and owned residual dispositions are complete.

- [ ] T010 Verify final production revisions and prepare review evidence.
  - Depends on: T004
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
  - Evidence: Pending.
  - Resolved decisions: D003 (initial candidate/policy); T010.1 must reconfirm final composition
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: Focused reachable commits and dated results exist; exact PR revisions, full
    build/test and final review evidence remain unverified. T006 lifecycle Memcheck attempts
    stopped before tests in Qt aeshash128/QDBus/QIBus startup (exit 99), including a compose
    input-method retry that still loaded IBus. Diagnose/revalidate the final runtime input-method
    profile here; owner local-developer. No lifecycle memory-clean claim or broad suppression.

  - [ ] T010.1 Confirm final production revisions and invalidate affected evidence.
    - Depends on: T004
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Reconfirm the T019 manifest against final production patches; enumerate changed
      source/dependency/build/fixture/environment inputs and invalidate/reconcile affected
      correctness, manual/visual, archive, lifecycle and benchmark results before sign-off.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - [ ] T010.2 Run pixi run build -j 2 and the affected focused suites on those revisions.
    - Depends on: T010.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Run pixi run build -j 2 and the affected focused suites on those revisions.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - [ ] T010.3 Run pixi run test and record any failure/skip with its acceptance impact.
    - Depends on: T010.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Run pixi run test and record any failure/skip with its acceptance impact.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

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
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

- [ ] T011 Promote performance contracts and route residual work.
  - Depends on: T010, T019, T021, T009
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `docs/reference/document-open-performance.md`,
    `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
  - Acceptance: All required original/added criteria and F001–F009 have verified or explicitly
    reviewed dispositions; no safety/acceptance blocker is hidden by completion. Verify
    B009/B010/B011 ownership and scope remain valid, promote durable truth and record actual
    review/closure/archive metadata. Deferred new investigations do not need execution for this core
    slice to close.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Pending.
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: No current closure or durable promotion claim; legacy plan/checklist remains
    historical.

  - [ ] T011.1 Promote accepted performance behavior/design and operational validation to durable targets.
    - Depends on: T010, T019, T021, T009
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `docs/reference/document-open-performance.md`,
      `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
    - Acceptance: Promote accepted performance behavior/design and operational validation to durable
      targets.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - [ ] T011.2 Route conditional GUI/startup, reduced-resolution and other residual work to owned destinations.
    - Depends on: T011.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `docs/reference/document-open-performance.md`,
      `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
    - Acceptance: Verify existing B009/B010/B011 routing, ownership and non-blocking scope; route
      any newly discovered residual to one owned destination without discarding required current
      correctness/performance acceptance.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - [ ] T011.3 Verify acceptance/review coverage and record actual closure/archive metadata.
    - Depends on: T011.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `docs/reference/document-open-performance.md`,
      `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
    - Acceptance: Verify acceptance/review coverage and record actual closure/archive metadata.
    - Evidence mode: artifact
    - Evidence: Pending.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

## Routed Follow-Ups — Outside Active Delivery

These are evidence-bearing routing records, not completed investigations. Their original conditional
acceptance is retained in the linked backlog. They are not prerequisites for core runtime or
closure; T011 audits routing/ownership.

- [>] T008 Decide whether GUI restore or startup profiling is warranted.
  - Depends on: none
  - Requirements: Requirement 3 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `docs/backlog/README.md`, `docs/backlog/spec003-follow-ups.md`, `verification.md`
  - Acceptance: Fresh timing either identifies one dominant substep with an owned follow-up or
    explicitly defers GUI restore/startup work; normal/clean profiles are distinguished.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: 2026-11-06 user-authorized replan: B010 is an owned deferred backlog item with original fresh-timing/normal-versus-clean-profile activation and exit checks. Core retained-optimization measurements remain T004.3. No GUI/startup investigation was executed.
  - Decision owner: local-developer
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Destination: docs/backlog/spec003-follow-ups.md#b010-conditional-gui-restore-and-startup-investigation
  - Status: Routed optional investigation; B010/local-developer owns future activation. Not a core completion dependency.
  - Owner: local-developer
  - Original prerequisite: Fresh T004 measurements before future B010 activation.
- [>] T007.2 Profile a larger representative document before proposing further batching or bulk deletion.
  - Depends on: none
  - Requirements: Requirement 6 (all acceptance criteria)
  - Files: `docs/backlog/README.md`, `docs/backlog/spec003-follow-ups.md`, `verification.md`
  - Acceptance: Profile a larger representative document before proposing further batching or bulk
    deletion.
  - Evidence mode: artifact
  - Evidence: 2026-11-06 user-authorized replan: B011 owns the original conditional larger-model profile before further tree batching/bulk deletion. Required current functional qualification remains T007.1 and retained-tree comparative evidence remains T004.3. No further-optimization profile was run.
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Destination: docs/backlog/spec003-follow-ups.md#b011-further-tree-batching-or-bulk-delete-profiling
  - Status: Owned optional routing outcome outside T007 current runtime acceptance; not a completed profile.
  - Owner: local-developer
  - Original prerequisite: T007.1 current qualification before future B011 profiling.
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
| F1, F2; restore ordering and lazy full-resolution prototype | T006 | Current local lifecycle qualification complete; final platform/runtime profile remains T010 |
| F2 reduced-resolution checkbox | T009 | Separate decision, not implemented |
| G1, G2; tree pending flag, side effects, larger trace | T007 | Batching exists; no bulk-delete bypass; larger-model evidence open |
| G3; startup tracing/ranking | T008 | Historical normal-profile observations; conditional follow-up |
| H1, H4; cleanup and commit separation | T010 | Reachable focused commits/absent switches are source evidence only |
| H2, H3; build, tests, PR gates | T010 | Current-revision final gates unverified |
| Promotion/closure not represented in legacy checklist | T011 | Required before closure |
