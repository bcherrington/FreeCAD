---
title: FreeCAD performance optimization tasks
doc_type: spec
artifact_type: tasks
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# Tasks

Current performance policy: **D007 cost-benefit review** in
[acceptance-policy-v2.md](acceptance-policy-v2.md). Original D001 v1 outcomes and
dated checkpoints below remain evidence; their numeric vetoes do not govern current
product decisions. Correctness/provenance/coverage gates remain mandatory.


## Context And Execution

Read requirements, design, baseline-status, traceability, verification and the
T019 validation manifest before acting. Task IDs remain stable while the file
is now arranged in the agreed delivery sequence. `Depends on` includes explicit
process sequencing as well as technical prerequisites; changing that order
requires a recorded replan, not selecting another ready task silently.

Completed T019/T021 provenance is retained. Current source presence and imported
historical checks live in [baseline-status.md](baseline-status.md), outside
execution-state accounting. At the 2026-11-06 replan, the five remaining qualification
checkpoints started pending without qualifying child/current runs; that historical
initial state does not override their current evidence/status below. Reserve `[/]`
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
| 2B — Tree | T014/T017/T007 complete | Receiver/completion fixes and current functional qualification |
| 2C — CAD maps | T015/T016/T005 complete | Bounded parsing and CAD reference identity qualified |
| 2D — Archives | T018/T002/T002.1/T002.2 complete | Archive error outcomes and compatibility qualified |
| 3 — Performance | T020 measurement complete → T022/D005 plan complete → T004.1/T004.2/T004.3 measurements/exclusions and T004.4 scoped review complete | Mandatory hard gates and D007 workflow/operating-scope dispositions |
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
T020 -> T022
T001, T002, T003, T005, T006, T007, T020, T022 -> T004
T004.1 -> T004.2 -> T004.3 -> T004.4 (required T004 completion slices)
T004 -> T010
T010, T019, T021, T009 -> T011
none -> T008
```

## Phase 1 — Finish Planning

Exit satisfied: T021/T019 evidence retained, T009 scope disposition recorded, and T001
resolved D001 with the original benchmark protocol. That was the earlier planning
checkpoint. Current next action is T011 after completed T010 Linux qualification;
completed T022/T004.1/T004.2/T004.3 measurements are not reopened.

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

- [x] T014 Bind tree application callbacks to widget lifetime.
  - Depends on: T006
  - Requirements: Requirement 6 AC3
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/Gui/Tree.cpp`, `tests/src/Gui/TreeWidgetLifetime.cpp`,
    `tests/src/Gui/CMakeLists.txt`, `t014-validation.json`, `verification.md`
  - Acceptance: Ensure both new start/finish-open subscriptions and queued work cease with
    TreeWidget lifetime using established scoped connections or explicit disconnection. Add actual
    destroy/recreate then application-open regression with stale receiver access detected; hiding a
    dock alone is insufficient.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: t014-validation.json and verification.md#t014-tree-widget-receiver-lifetime bind actual committed T006 root 99b341fa31/docs 413cefe plus exact Tree.cpp/CMake/new-test diff, source/build/runtime/log identities, 7 clean dependencies and 510 unchanged packages. Old code fails both direct/parent destruction rows, retaining start/finish subscribers; separate real App open after heap receiver deletion reproduces Memcheck invalid write in slotStartOpenDocument, exit 99. Two explicit destructor disconnections correct it without Tree.h/public API/scheduler changes. All 4 functional rows (6 Qt passes including setup/cleanup) pass normally and under Memcheck with zero errors, one exact existing libunwind probe suppression, leak/undefined-value checks disabled. Three replacement cycles exercise six actual App opens; owned timers and queued production status calls drain after receiver deletion. Full pixi build, five focused CTest targets and scoped pre-commit pass, zero failures/skips. Direct checker verifies exact hashes, 40 stable IDs, 13 prior completed task records, 89 links/anchors, immutable artifacts/receipts, CRLF and unrelated console preservation; package lint/context zero diagnostics/gaps. Compose input-method profile explicit; default IBus and final Windows/native qualification remain T010. Broader restore completion/object effects remain T017/T007.
  - Owner: local-developer
  - Properties: CP-003
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F003 reproduced by retained start/finish subscriber counts after direct/parent
    destruction and by a Memcheck invalid write during actual App open after heap receiver deletion.
    Two explicit destructor disconnections correct it; Tree.h, public API and scheduler policy
    remain unchanged. See verification.md#t014-tree-widget-receiver-lifetime and t014-validation.json.

  - Status: Completed local Requirement 6 AC3 receiver-lifetime correction and qualification; F003 locally resolved. Next T017; T007 broader tree and T010 final/platform acceptance remain required.
- [x] T017 Match tree restore suppression to all completion paths.
  - Depends on: T014
  - Requirements: Requirement 6 AC1, AC4
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/Gui/Tree.cpp`, `tests/src/Gui/TreeRestore.cpp`, `tests/src/Gui/CMakeLists.txt`,
    `t017-validation.json`, `verification.md`
  - Acceptance: Define matching completion or safe rescheduling for app open, direct
    doc.load/doc.restore, import/copy-paste and failed restore. Narrow suppression to an owned
    operation or provide broader completion hooks based on source evidence. With timers initially
    idle, require tree population/status/observer updates without another edit/open and no stranded
    pending state on failure.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: t017-validation.json and verification.md#t017-tree-restore-completion bind committed T014 root 3587399d1b/docs c79bd6e plus exact Tree.cpp/CMake/new-test diff, source/build/runtime/log hashes, 7 clean dependencies and 510 matching packages. Final old-code matrix has 5 Qt passes/7 failures (zero-timeout, empty-app-open, direct/Python/delayed restore, merge import, failed-direct observer cleanup); populated app-open, copy-paste and failed-app-open controls pass. Initial C-API harness GIL fault was diagnosed/corrected before final red run; import source remains alive during completion assertions. Existing owned timer now remains scheduled during suppression, minimum 1ms retry while restoring; app finish-open preserves pending until actual flush, and restoring document creation schedules observer-only completion. No Tree.h, App/Gui document ordering/flags, public API/layout or extra subscription change. All 10 functional rows (12 Qt passes) pass normally and under Memcheck with zero memory-access errors and one exact existing libunwind probe suppression; leak/undefined-value checks disabled. Full pixi build, 6 focused CTest targets and scoped pre-commit pass. Direct checker verifies hashes, 40 stable IDs, 14 prior completed task records, 90 links/anchors, frozen receipts/artifacts, CRLF and unrelated console preservation; lint/context zero diagnostics/gaps. Larger-model, selection/focus/per-object qualification remains T007; final/default-IBus/Windows platform applicability T010.
  - Owner: local-developer
  - Properties: CP-003
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F005 reproduced from idle timers: final old-code matrix has 5 passes and
    7 failures across direct/load/delayed/import/zero-timeout, empty-document observer attachment
    and failed-direct cleanup. Existing owned timer now resumes after restore guards exit, with
    immediate app-open flush retained. App/Gui document ordering, Tree.h and public API unchanged.
    See verification.md#t017-tree-restore-completion and t017-validation.json.

  - Status: Completed local Requirement 6 AC1/AC4 correction and focused idle restore/load/import/error completion; F005 locally qualified. Next T007; broader tree, archive and final-platform/performance gates remain required.
- [x] T007 Validate tree status batching and larger-model behavior.
  - Depends on: T014, T017
  - Requirements: Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `tests/src/Gui/TreeBehavior.cpp`, `tests/src/Gui/CMakeLists.txt`,
    `tests/src/Gui/data/tree-fixtures.md`, `tests/src/Gui/data/tree-8.FCStd`,
    `tests/src/Gui/data/tree-512.FCStd`, `t007-validation.json`, `verification.md`
  - Acceptance: Requirement 6 AC1–AC4 pass after T014/T017 on representative small and larger pinned
    documents: receiver destruction/recreation and every suppressed
    open/direct-load/restore/import/paste/error path complete safely from idle timers without
    unrelated edit/open; selection/focus/observers and per-object effects remain intact. T007.2 is
    an explicitly routed future-optimization outcome at B011, not a required current runtime check;
    retained tree comparative evidence remains T004.3.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: Required T007.1 completed; T007.2 remains unchanged follow_up at owned/deferred B011, no future profile executed. t007-validation.json binds committed T017 root da90da6f93/docs 68370d7 plus exact test/registration/retained-input diff, unchanged production sources, frozen validation binary identities, 7 clean dependencies and 510 matching packages. Checksum-pinned 10/514-object nested-group/image-plane archives qualify current small/larger tree behavior. Native suite 27 Qt passes (25 functional cases), 7 CTest targets, full pixi build, scoped pre-commit and 7-case Memcheck subset (9 Qt passes, zero access errors) pass. Restoring/idle timer, selection/focus/deletion/reparenting/map/observer, expansion/reload and internal drag/drop assertions pass; actual receiver destruction/recreation and current T014/T017 reruns included. Initial harness service/activation/asynchronous assertion gaps corrected; overlapped-build memory attempt discarded; final source/binaries frozen and verified unchanged. Direct checker passes exact hashes, 40 stable tasks, 15 prior completed records, 91 links/anchors, immutable receipts/protocol/history, B011 routing and unrelated console preservation; lint/context zero diagnostics/gaps. T004.3 retains predeclared performance comparisons; T010 retains final/Windows/default-IBus/mixed-workbench applicability.
  - Status: Completed current local Requirement 6 AC1–AC4 and CP-003 tree checkpoint; Phase 2B complete. Next T015; B011 remains deferred and broader performance/final delivery gates remain required.
  - Properties: CP-003
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Baseline: baseline-status.md#tree-status-batching
  - Required subtasks: T007.1
  - Routed subtasks: T007.2 at B011; verify owned routing, not future profile execution.

  - [x] T007.1 Verify pending-status flush and open/close/reload, drag, expansion, selection and observer behavior.
    - Depends on: T014, T017
    - Requirements: Requirement 6 (all acceptance criteria)
    - Files: `tests/src/Gui/TreeBehavior.cpp`, `tests/src/Gui/CMakeLists.txt`,
      `tests/src/Gui/data/tree-fixtures.md`, `tests/src/Gui/data/tree-8.FCStd`,
      `tests/src/Gui/data/tree-512.FCStd`, `t007-validation.json`, `verification.md`
    - Acceptance: Verify open/close/reload and idle direct doc.load/doc.restore/import/paste/error
      completion, eventual population/status/observer updates without another edit/open, and actual
      tree destruction/recreation followed by app signals. Preserve drag, expansion, selection and
      focus. Include small and larger pinned documents for current functional/observer correctness;
      later optimization profiling is separate B011 work.
    - Evidence mode: artifact
    - Evidence: 2026-11-06: t007-validation.json and verification.md#t007-tree-functional-qualification bind current source/build/runtime and checksum-pinned 10/514-object FCStd inputs. New TreeBehavior suite passes 25 functional cases (27 Qt passes) natively on xcb/Xvfb/Metacity and in the seven-target CTest run. Nine restore/open/load/import/copy/error rows repeat at each size; per-object deletion preserves another selection and Qt focus, group deletion reparents every child and preserves map/view-provider and label observer behavior. Expansion/labels survive save/reload/close/open and real receiver recreation; controlled DraggingState defers/resumes work, production drop handler moves selected root object into group with eventual tree agreement. Seven focused cases including both sizes of deletion/reload/drop and larger import pass Memcheck (9 Qt passes), zero access errors, one exact existing libunwind probe suppression; leak/undefined-value checks disabled. Full pixi build and scoped pre-commit pass. Production source and earlier tests unchanged; fixture harness initializes full Gui services/NoneWorkbench and uses child-tree panel ownership. Current lifetime/restore/image/selection tests rerun. Final profile/mixed-workbench applicability remains T010; no performance or cross-application drag claim.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

The conditional T007.2 investigation is preserved under Routed Follow-Ups;
it is excluded from this parent's current runtime completion gate.

### 2C — Numeric Parsing And CAD Identity

Exit: bounded conversion, writer/alias/child identity and dependent CAD target qualification are
evidenced before T005 sign-off.

- [x] T015 Bound element-map numeric parsing and define compatibility.
  - Depends on: T007
  - Requirements: Requirement 4 AC1, AC2
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/App/ElementMap.cpp`, `src/App/ElementMapRestore.h`,
    `tests/src/App/ElementMap.cpp`, `t015-validation.json`, `design.md`, `verification.md`
  - Acceptance: Document numeric conversion as part of the change; preserve approved legacy strtol
    tolerance or record an intentional compatibility decision. Test decimal/hex limits,
    LONG_MIN/LONG_MAX, overflow, signs, prefixes, whitespace, invalid suffixes and oversized tokens;
    no signed overflow, invalid access or uncontrolled partial acceptance.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: 2026-11-06: t015-validation.json and verification.md#t015-element-map-numeric-restoration bind root 09c9edc703/docs a38de02 plus exact tracked/private-header diff, legacy strtol source, final source/build/runtime/probe/log hashes, 7 clean dependencies and 510 matching packages. Old converter reproduced 7/8 new-case failures and UBSan signed overflow at LONG_MIN; caller regressions reproduced an IndexedName assertion abort and failed child/count validation; Memcheck exposed a zero-index colon name borrowing freed prefix storage. Checked unsigned conversion preserves legacy whitespace/sign/hex-prefix/numeric-suffix/zero/range-clamp values without unbounded reads. Unrepresentable indices/counts/ranges reject before narrowing/arithmetic/allocation; colon name compact fixes lifetime. Final full pixi build, 277 naming/hash tests, 290 CTests, all 30 focused cases under Memcheck (zero errors; one exact libunwind probe suppression; leak/undefined-value checks disabled), exact-header ASan/UBSan and scoped pre-commit pass. Final sources/binaries unchanged across validation. Direct checker passes 40 stable IDs, 17 prior completed records, 93 links/anchors, immutable artifacts/receipts/protocol, original line endings and unrelated console preservation; lint/context zero diagnostics/gaps. Shared external coordination path recorded/acknowledged; native geometry-read profiling remains separate. Broader writer/alias/child/dependent CAD identity remains T016/T005; no format migration, installed-runtime or performance fix claim.
  - Owner: local-developer
  - Properties: CP-005
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F004 numeric overflow reproduced by UBSan and legacy-oracle tests; mapped
    index narrowing asserted, invalid child ranges/counts escaped validation, and zero-index colon
    names borrowed a freed prefix vector under Memcheck. T015 corrects/qualifies these bounds and
    ownership paths without changing valid writer format. Broader identity remains T016/T005.

  - Status: Completed local numeric compatibility, range and boundary-ownership correction for Requirement 4 AC1/AC2; F004 broader identity remains open. Next T016; final platform/composition and performance gates remain required.
- [x] T016 Qualify element-map and dependent CAD reference identity.
  - Depends on: T015
  - Requirements: Requirement 4 AC1, AC3, AC4
  - Design: design.md#moe-safety-and-compatibility-contracts
  - Files: `src/App/ElementMap.cpp`, `tests/src/App/ElementMap.cpp`,
    `tests/src/Mod/PartDesign/App/ElementMapDocumentIdentity.py`, `verification.md`
  - Acceptance: Cover semicolon/colon/dollar-hashed forms, decimal child SIDs versus hexadecimal
    mapped SIDs above nine, multiple IDs/aliases and nested/shared children. Verify semantic
    save–restore–save, both lookup directions after input buffers die, and an existing CAD document
    with dependent subelement references through reopen/recompute; record fixture identities.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: 2026-11-07: t016-validation.json and verification.md#t016-element-map-and-dependent-cad-identity bind b086a0bb2f/docs 7ff7e15 plus exact C++/new Python diff, source/build/runtime/fixture identities, seven clean dependencies and 510 matching packages. Four new cases qualify writer semicolon/colon/dollar-hash forms, IDs 11/13, aliases/multiple SIDs, nested/shared maps, semantic roundtrips and both lookup directions after stream buffers die. Missing child-SID hasher reproduced SIGSEGV (-11); guarded warning/omission now matches mapped SIDs. Unknown/malformed child IDs preserve valid IDs. Checksum-pinned ModelFromV021.FCStd retains four independently specified Chamfer.Base edge targets across two map-bearing save/reopen cycles and three explicitly dirtied recomputes. Final full Pixi build, 281 App tests, 294 CTests, CAD harness, 34 Memcheck cases (zero errors; one exact libunwind probe suppression; leaks/undefined-value checks disabled) and scoped pre-commit pass. Requirement 4/CP-005 aggregate checkpoint remains T005; final/platform/performance gates remain.
  - Owner: local-developer
  - Properties: CP-005
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F004 fixture review found current one-child parser regression does not
    establish all writer forms or dependent-feature identity.

  - Additional audit disposition: T016 reproduced a missing child-SID hasher SIGSEGV, then
    added warning/omission handling matching mapped SIDs. Missing/malformed-ID regressions pass;
    no valid archive writer or public API change. T015 evidence remains historical and unchanged.

  - Status: Completed local identity and missing-hasher correction; T005 aggregate compatibility checkpoint is next. No archive format/API or performance claim.
- [x] T005 Verify element-map parser compatibility.
  - Depends on: T015, T016
  - Requirements: Requirement 4 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/App/ElementMap.cpp`, `tests/src/App/ElementMap.cpp`
  - Acceptance: Requirement 4 AC1–AC4 and CP-005 pass after T015/T016: bounded numeric
    grammar/legacy tolerance, semantic save–restore–save, bidirectional lookups after input-buffer
    release and dependent CAD subelement targets after reopen/recompute.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: 2026-11-07: t005-validation.json and verification.md#t005-aggregate-element-map-parser-compatibility record 75 direct identity/integrity/applicability checks at committed T016 root 95d1df147f/docs e3bf027. All Requirement 4 AC1–AC4 and CP-005 local criteria map to executed T015/T016 proof: bounded legacy numeric behavior and unchanged exact-header ASan/UBSan, writer forms/aliases/multiple SIDs/nested shared maps and post-buffer bidirectional lookups, four independent CAD edge targets across two save/reopen cycles and three forced recomputes. Exact committed source diff/final binaries, raw logs/probe, seven clean dependency heads, 510 packages, CMake cache/lock and fixture hashes match. T016 already executed full build, 281 App tests, 294 CTests, 34 Memcheck cases and CAD harness on this unchanged implementation. No new build/runtime rerun or loaded-image equivalence claimed. Requirement 4/CP-005/F004 complete locally; T010 final/platform and T004 performance remain.
  - Status: Completed aggregate compatibility acceptance; Phase 2C complete. Next T018 document-level archive failure recovery; final production/platform, performance and promotion gates remain.
  - Properties: CP-005
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Baseline: baseline-status.md#element-map-parsing

### 2D — Archive Failure Recovery And Compatibility

Exit: document-level failure outcomes and included-file compatibility pass before comparative
measurements.

- [x] T018 Specify and verify document-level archive failure recovery.
  - Depends on: T005
  - Requirements: Requirement 1 AC1–AC3
  - Design: design.md#archive-failure-compatibility-policy
  - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
    `tests/src/Base/Reader.cpp`, `tests/src/App/PropertyFileIOFaults.cpp`,
    `tests/src/App/PropertyFileIOFaults.h`, `tests/src/App/CMakeLists.txt`, `verification.md`
  - Acceptance: Record an approved outcome table for empty/nonempty read/write/close failures:
    diagnostics/failure lists, partial destination bytes/permissions, property notifications, and
    later-entry restoration. Exercise a failing first registered entry followed by a valid second at
    document level; preserve existing policy or explicitly approve/document a change rather than
    relying on a hook exception.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: 2026-11-07: t018-validation.json and verification.md#t018-document-level-archive-failure-recovery bind root 95d1df147f/docs e3bf027 plus exact test/CMake/Linux shim diff, frozen final source/binary/config/log/fixture identities, seven clean dependencies and 510 packages. Preserved existing policy in design.md#archive-failure-compatibility-policy under delegated local T018 authority; no production/API/format change. Twelve full Document::restore rows cover empty/nonempty read/write/close/create faults, success and read-only reuse; verify real live FailedFiles, log/error diagnostics, partial bytes/permissions, payload notifications, completed restore flags and valid later-entry bytes/read-only state. Fault hits prove actual injection; empty write correctly has zero attempts. Rejected assignment retains bytes/value/permissions/no notifications but updates OriginalName; empty/nonempty missing-payload save throws, preserves old archive/path and leaves UUID temp. Full Pixi build, 303 App cases, 26 Reader cases, 342 CTests and 22 Memcheck cases pass; zero memory errors, one existing libunwind probe suppression, leak/undefined-value checks disabled. Failed reporting build/stale-binary runs explicitly discarded and retained. T005 documentation/receipt and prior history preserved; T002 aggregate compatibility remains next.
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

  - Audit disposition: Preserve existing rejected-assignment OriginalName update and missing-payload
    save exception/previous-archive preservation/UUID temporary residue; no transactional metadata
    or archive cleanup behavior change. Linux test shim never links to production targets.

  - Status: Completed local preserved failure policy/matrix and metadata/save audit. Next T002 aggregate included-file compatibility; Windows/other libc, final production/platform and performance/promotion remain.
- [x] T002 Verify included-file restore compatibility on the target revision.
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
  - Evidence: 2026-11-07: T002.1/T002.2 complete. t002-validation.json and verification.md#t002-included-file-compatibility-checkpoint reconcile all Requirement 1 AC1–AC3 and CP-001 at ad1bf7eac8/docs ee77d7f plus exact new-test diff. Four raw ZIP/application-open byte/size boundaries, six direct hook cases and twelve current T018 full-document rows establish bytes, entry boundaries, notifications/read-only behavior, observable failures, partial output and later-entry continuation. 45 integrity/applicability checks bind unchanged production/shim/configuration, retained T018 test bodies/raw artifacts, seven clean dependency heads, 510 packages and frozen current source/binaries/fixtures. Full Pixi build, 310 App cases, 26 Reader cases, 349 CTests and 29 included-file Memcheck cases pass; zero errors with existing libunwind suppression and leak/undefined-value limits. No production behavior/API/archive-policy/performance claim. Local Requirement 1/CP-001/F006 and Phase 2 correctness complete; next T020, with T010 final/platform and T011 promotion pending.
  - Status: Completed local aggregate included-file compatibility and both child tasks. Phase 2 complete locally; next T020 deferred first-frame latency/image memory under D001/T019. Final production/platform, performance and promotion gates remain.
  - Properties: CP-001
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Baseline: baseline-status.md#included-file-restore

  - [x] T002.1 Verify empty, binary, multi-buffer and multi-entry restored bytes and ZIP boundaries.
    - Depends on: T018
    - Requirements: Requirement 1 (all acceptance criteria)
    - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
      `tests/src/Base/Reader.cpp`
    - Acceptance: Verify empty, binary, multi-buffer and multi-entry restored bytes and ZIP
      boundaries.
    - Evidence mode: artifact
    - Evidence: 2026-11-07: t002-validation.json and verification.md#t002-included-file-compatibility-checkpoint bind ad1bf7eac8/docs ee77d7f plus exact native-test diff and frozen final source/build/runtime/fixture identities. New archiveBoundariesAndNormalApplicationOpen compares raw ZIP entry sizes/full bytes for 0/65536/65537/131089-byte binary payloads, then normal Application::openDocument restores regular files with exact sizes/bytes and read-only permissions, including empty file. Actual archive and deterministic payload SHA256 identities recorded. Current full Pixi build, 310 App/26 Reader/349 CTest and all 29 included-file Memcheck cases pass; no production/API/format change.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

    - Status: Completed current local empty/binary/buffer-boundary/multi-entry byte and ZIP compatibility; final platform applicability remains T010.
  - [x] T002.2 Verify property notifications, read-only destination behavior and read/write/close failures.
    - Depends on: T002.1
    - Requirements: Requirement 1 (all acceptance criteria)
    - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
      `tests/src/Base/Reader.cpp`
    - Acceptance: Verify read/write/close failures, notifications and read-only destinations with
      both property-hook checks and T018 document-level failure outcomes; a failing first entry must
      not hide the expected valid second-entry behavior.
    - Evidence mode: artifact
    - Evidence: 2026-11-07: t002-validation.json records six new direct PropertyFileIncluded::RestoreDocFile cases for empty/nonempty write/close/bad-reader conditions, observable FileSystemError and payload notifications/regular-file existence/permissions/fault hits. Empty write is a successful zero-write control. All twelve T018 full-document rows rerun on the final frozen binary verify live failure lists, log/error diagnostics, partial outputs and correct later-entry bytes/read-only state; established policy retained. All 29 included-file cases pass normally and under Memcheck (zero errors; one exact libunwind probe suppression, leak/undefined-value checks disabled). Broader 310 App/26 Reader/349 CTest checks pass; receipt binds current fixture/source/build/log identities.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

    - Status: Completed local direct-hook plus document-level failure/notification/read-only compatibility; final platform applicability remains T010.
## Phase 3 — Qualify Comparative Performance

2026-11-07 evidence-driven replan: T020 completed measurement/disposition but P07
failed D001 and controls did not establish reliable acceptance. Mandatory T022/D005
now precedes T004 because a material rework/behavior/requirements decision may
change the source composition and invalidate subsequent comparisons. This does
not waive Phase 3 exit or reopen completed Phase 2 semantic correctness.

2026-11-08: T022/D005 planning is complete under t004-execution-plan.md. Retain P07
with unchanged D001 budgets. T004.1 and T004.2 comparable measurements/dispositions
are complete; T004.3 parser/save comparisons and the reviewed P08 exclusion are complete.
T004.4 resolves all eight D007 workflow/operating-scope dispositions; v1 labels are
preserved. Phase 3 local review is complete; final composition/platform and SC-005
acceptance remain T010.

Exit: T020/T004 provide complete applicable comparison evidence, preserve D001 v1
statistics, pass mandatory correctness/provenance/coverage gates and resolve D007
workflow cost-benefit/operating-scope reviews. Earlier source-fix tests do not prove speedup.

- [x] T020 Measure deferred first-frame latency and image memory.
  - Depends on: T001, T002, T003, T005, T006, T007
  - Requirements: Requirement 3 AC3, AC4; Requirement 2 AC1; Requirement 5 AC1–AC4
  - Design: design.md#evidence-applicability-and-benchmark-contract
  - Files: `verification.md`, `benchmark-protocol.md`, `performance-results.md`, `performance-results.json`, `src/Gui/ViewProviderImagePlane.cpp`,
    `src/Gui/BitmapFactory.cpp`, `performance-recovery-2-results.md`,
    `performance-recovery-2-results.json`, `t020-validation.json`
  - Acceptance: Under D001 pinned fixtures/endpoints, measure command restore, GUI
    open-to-first-correct-frame, hidden-first-show-to-frame and repeat-show. Record initial
    visibility, pending/materialized textures, peak process/decoded/Coin bytes for direct and
    1/2-component fallback formats; disposition material latency/memory regressions and separate
    shifted from eliminated work. Do not claim GPU residency from CPU bytes.
  - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
    commands, results and limits in verification.md.
  - Evidence mode: artifact
  - Evidence: 2026-11-07: Measurement/disposition complete. Corrected campaign completed all seven cells once with336 valid fresh-process observations,20 measured pairs percell, zero worker failures/retries/exclusions/pooling. performance-results.md/json aggregate and performance-recovery-2-results.md/json bind exact raw/context/statistical/Q/freeze/controller/audit provenance; first-campaign result/report/receipt bytes and frozen decision-ledger binding are explicitly archived and hashed. First-show857.91/1843.09ms fails500/1000ms; all-visible p95 and PartDesign median fail NR; remaining timing inconclusive; all resource limits pass. Deferred versus avoided work, pending/materialized/load counts, direct/fallback absolute storage, full-resolution/calibration and no GPU-residency inference are explicit. Both42,290-hash audits,551 focused tests perarm,14 frozen-context checks,six ownership controls,20 synthetic tests,three optimized-controller checks,privacy/immutable-protocol/CRLF checks,scoped hooks and final independent review pass. P07 NOT ACCEPTED underD001; mandatory T022/D005 beforeT004/T004.1 owns material decision, measurement conditions, source/evidence and affected revalidation. Phase3/SC005/T004/T010 acceptance remains unsatisfied; no behavior change, accepted tradeoff or thresholdwaiver chosen.
  - Owner: local-developer
  - Resolved decisions: D001, D004
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: F007: show performs deferred decode/conversion and fallback storage expands
    components; complete measurement/disposition is recorded in performance-results.md; P07 is rejected and residual ownership is mandatory T022/D005 before T004.

  - Status: Completed measurement and regression disposition only. P07 NOT ACCEPTED; Phase3 exit unsatisfied. Mandatory T022/D005 precedes T004 and production acceptance; no shipping or speedup claim.
  - Decision owner: local-developer
- [x] T022 Plan validation and first-use criteria for retained P07 before T004.
  - Depends on: T020
  - Requirements: Requirement 3 AC1–AC5; Requirement 5 AC1–AC4
  - Design: design.md#evidence-applicability-and-benchmark-contract; design.md#open-questions
  - Files: `open-decisions.md`, `design.md`, `requirements.md`, `tasks.md`, `traceability.md`,
    `verification.md`, `benchmark-protocol.md`, `performance-recovery-2-results.md`,
    `t004-execution-plan.md`, `t004-comparison-readiness.json`
  - Acceptance: Preserve the owner's 2026-11-08 choice to retain lazy full-resolution P07.
    Define reliable validation of opening-time/RAM benefits and a bounded first-use/tail,
    repeat-display and responsiveness plan. Agree any future median/p95 criteria explicitly;
    no exact replacement budget is assumed. Record necessary source composition, rework and
    affected revalidation before T004/T004.1; no old-run retry, pooling or retroactive waiver.
  - Validation: Decision discovery and task contexts must expose accurate D005 status; after
    resolution it must disappear from unresolved discovery. T004 and children must not become
    executable merely because T020's evidence task is complete. Review the chosen source,
    requirement, safety and measurement/revalidation plan before implementation or new sampling.
  - Evidence mode: artifact
  - Evidence: 2026-11-08: D005 resolves retained full-resolution P07 with unchanged D001 criteria under the user's retention/T004 instructions. t004-execution-plan.md defines exact common613def, eight-file source equivalence, all-change construction/ownership, bounded16worker exploratory diagnostics, observed host controls, concrete correction/new-decision requirement and affected revalidation before any fresh seven-cell P07 campaign. t004-comparison-readiness.json verifies eight public fixtures and eight origin deltas; source-only P01 draft reviewed. Independent t004_plan_review reports no remaining blocker after header/diagnostic-arm corrections. No new budget, product behavior, native measurement, waiver or P07 acceptance.
  - Owner: local-developer
  - Decision owner: local-developer
  - Resolved decisions: D005
  - Status: Technical plan complete; D005 resolved. T004.1 construction/qualification may begin, with sampling gated by exact new arm/build/probe/fixture qualification. P07 remains NOT ACCEPTED.

- [x] T004 Remeasure restore and image conversion comparably.
  - Depends on: T001, T002, T003, T005, T006, T007, T020, T022
  - Requirements: Requirement 1 AC1–AC3; Requirement 2 AC1–AC3; Requirement 3 AC1–AC8; Requirement 4
    AC1–AC4; Requirement 5 AC1–AC4; Requirement 6 AC1–AC4
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `verification.md`, `benchmark-protocol.md`, `performance-results.md`, `performance-results.json`,
    `t004-execution-plan.md`, `t004-comparison-readiness.json`, `acceptance-policy-v2.md/json`,
    `cost-benefit-review.md/json`, `d007-validation.json`,
    `t0044-cost-benefit-review.md/json`, `t0044-validation.json`
  - Acceptance: Applicable comparison evidence and D007 cost-benefit dispositions cover the
    pinned source/fixture/build composition and every retained restore/save, parser, image/lazy
    and tree change or explicit reviewed exclusion. Preserve all D001 v1 outcomes. Mandatory
    correctness/provenance/coverage pass independently; review first/repeat-show, complete
    workflows, tails, material memory/concurrency and operating scope in T004.4.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: 2026-11-09: T004.1/T004.2/T004.3 provide complete applicable scoped comparisons or reviewed exclusions; t0044-cost-benefit-review.md/json and t0044-validation.json complete all P01-P08 D007 workflow/operating-scope dispositions after independent CAD, performance/governance and operations/docs review. Historical D001 outcomes and all26 bound source artifacts remain unchanged; eight exact T004.3 completion-time authority files archived. Local hard gates remain qualified by original receipts, explicit P05 memory accounting/headroom and P07 first-use tradeoffs accepted in selected scopes. No new benchmarks or production approval. Durable reference and runbook record lasting decisions. T010 owns exact final source/build/platform applicability and SC-005 final acceptance; T011 owns closure.
  - Resolved decisions: D001, D007
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Context evidence: Historical May restore and image improvements cannot establish current
    performance acceptance.

  - Status: Complete applicable local measurement/exclusion and D007 scoped cost-benefit review. All four children complete. Next T010.1; final composition/platform, SC-005 and whole-spec production acceptance remain open.
  - [x] T004.1 Record comparable restore/open benchmark runs under D001.
    - Depends on: T001, T002, T003, T005, T006, T007, T020, T022
    - Requirements: Requirement 1, Requirement 2; Requirement 3 AC1–AC6 (completion-time measurement contract)
    - Files: `verification.md`, `benchmark-protocol.md`, `performance-results.md`, `performance-results.json`,
      `src/App/PropertyFile.cpp`, `tests/src/App/CMakeLists.txt`, `tests/src/App/PropertyFile.cpp`,
      `tests/src/App/IncludedFilePerformanceTrace.h`, `tests/src/App/IncludedFilePerformanceTrace.cpp`,
      `tests/src/App/IncludedFilePerformanceProbe.py`, `tests/src/Gui/run_image_performance_probe.py`,
      `benchmark-assess.py`, `test_benchmark_assess.py`
    - Resolved decisions: D001, D005, D006
    - Acceptance: Record comparable restore/open benchmark runs under D001.
    - Evidence mode: artifact
    - Evidence: 2026-11-08: t004-p01-results.md/json and fully hash-bound t004-p01-evidence record all five one-shot cells/240 fresh observations,20 measured pairs percell, zero worker failures/retries/exclusions/pooling. Exact freeze638a67eabe24d72a948fda0abfc2e9a073b4147ea2627bc191e840f0e3e484bc; both39,805-hash/510-live-package audits pass. Botharms31 included/probe+26Reader tests and10runtime contexts pass; native counters/bytes/permissions/fullmodel identity/recompute/parser provenance enforced perrecord. PrimaryPASS payload513.92→26.79ms,consoleopen1183.27→704.44ms; BIMFAIL openp95, ArchDetail/PartDesigninconclusive, EnginePASS; resourcesallpass. P01NOTACCEPTED overall under unchangedD001. ActiveT004 owns residual; at that T004.1 checkpoint T004.2 was next. Independent review recomputed all5assessments byte-equivalent; prospective task review distinguishes completedmeasurement from numericacceptance. Exact frozen task/ledger and originalT020aggregate bytes archived with explicit bindings; no timing reinterpretation.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

        - Status: Completed comparable measurement/disposition only. P01 NOT ACCEPTED overall; T004 remains in progress and owns P01 acceptance residual. At that checkpoint T004.2 was next; it is now complete and T004.3 is next. P07 remains retained/unaccepted.
  - [x] T004.2 Record comparable image conversion/lifecycle timings and remaining bottlenecks under D001.
    - Completion boundary: 2026-11-08 prospective reconciliation in `t004-conversion-plan.md` and `t004-conversion-reconciliation.json` supersedes the historical execution-plan prohibition for this measurement slice only. All 13 P04/P05 cells require comparable results or pre-sample reviewed dispositions; parent T004 owns every numeric failure/inconclusive outcome and P01/P07 acceptance. No D001 waiver.
    - Depends on: T004.1, T020
    - Requirements: Requirement 1, Requirement 2; Requirement 3 AC1–AC6 (completion-time measurement contract)
    - Files: `t004-conversion-results.md/json`, `t004-conversion-evidence/`, `t004-conversion-validation.json`,
      `t004-conversion-plan.md`, `t004-conversion-reconciliation.json`, `t004-p05-launch-plan.md`,
      `tests/src/Gui/ImageConversionPerformanceProbe.cpp`, `run_image_conversion_comparison.py`,
      `test_image_conversion_comparison.py`, `image_performance_namespace.py`,
      `verification.md`, `performance-results.md`, `t004-p07-diagnostic-results.md/json`,
      `t004-p07-diagnostic-validation.json`, `t004-p07-diagnostic-evidence/`,
      `t004-p07-first-show-breakdown.svg`, `t004-p07-corrected-diagnostic-plan.md`,
      `t004-runtime-forensics-plan.md`, `t004-runtime-forensics-results.json`,
      `src/Gui/BitmapFactory.cpp`, `src/Gui/ViewProviderImagePlane.cpp`,
      `tests/src/Gui/ImagePerformanceTrace.h/.cpp`, `ImagePerformanceTraceTest.cpp`,
      `ImagePerformanceProbe.py`, `benchmark_host_telemetry.py`, `run_image_diagnostic.py`,
      `launch_image_diagnostic.py`, associated focused Python tests,
      `storage-retirement-20261108.json`, `docs/runbooks/freecad-benchmark-storage.md`
    - Acceptance: Record comparable conversion and T020 GUI
      first-frame/hidden-first-show/repeat-show latency, initial visibility, pending/materialized
      texture counts, process/decoded/Coin image memory and D001 non-regression decisions for direct
      and 1/2-component fallback fixtures.
    - Evidence mode: artifact
    - Evidence: 2026-11-08: t004-conversion-results.md/json, t004-conversion-validation.json and its evidence manifest cover all 13 P04/P05 cells: 624 fresh workers (520 measured, 78 warm-ups, 26 diagnostics), 20 measured pairs per cell. All pixel/orientation/ownership/runtime checks pass; zero worker failures, retries, replacements, exclusions or pooling. P04 pre/post 31,256-binding and P05 pre/post 31,350-binding audits pass. All 485 scoped native and 56 Python cases pass. Every timing numeric threshold passes but unchanged variability rules make timing inconclusive. All five large P05 cells fail RSS/decoded memory (+64 MiB temporary); three also fail Coin (+48/+32/+32 MiB); all five small controls pass resources. Independent final review recomputed metrics/manifests and cleared privacy/provenance; asymmetric comparator coverage and P04 actual-interpreter equivalence are disclosed. Original T020 GUI and exploratory P07 evidence remain separate. Completion follows the prospective measurement/disposition reconciliation; parent T004 owns all P01/P04/P05/P07 numeric residuals. No D001 waiver; T004.3 is next.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

    - Status: Complete comparable measurement/disposition under prospective reconciliation. Parent T004 remains in progress for all numeric acceptance residuals; T004.3 next; no production acceptance.
  - [x] T004.3 Record per-change parser and tree comparison or reviewed exclusions.
    - Policy gate: Before any new samples, freeze acceptance-policy-v2.md/json (D007), the
      per-change review rubric and absolute timing/resource/coverage inventory in the execution
      plan. Wire policy-hash verification and separate v1 statistical output from D007 product
      dispositions. Do not use the unchanged v1 assessor alone as release approval.
    - Depends on: T004.2, T005, T007
    - Requirements: Requirement 3 AC1–AC5, AC7, AC8 (pre-sample policy; final review T004.4); Requirement 4 AC1–AC4; Requirement 6 AC1–AC4
    - Design: design.md#evidence-applicability-and-benchmark-contract
    - Files: `t0043-results.md/json`, `t0043-evidence/`, `t0043-validation.json`,
      `t0043-execution-plan.md`, `tests/src/App/DocumentPerformanceTrace.*`,
      `DocumentPerformanceProbe.py`, `document_performance_fidelity.py`,
      `run_document_performance_probe.py`, `run_document_performance_comparison.py`,
      `verification.md`, `benchmark-protocol.md`, `acceptance-policy-v2.md/json`,
      `cost-benefit-review.md/json`, `performance-results.md`, `performance-results.json`
    - Acceptance: Assign comparative baseline/change evidence to retained parser/tree optimizations
      and any buffered-save patch. Record per-change build/revision/fixture provenance and
      attribution limits; a combined document-open gain cannot silently stand in for each change.
      Any exclusion has a reviewed rationale and one owned residual destination.
    - Validation: Run the focused regression cases in acceptance on the T019 pinned revision; record
      commands, results and limits in verification.md.
    - Evidence mode: artifact
    - Evidence: 2026-11-08: t0043-results.md/json, t0043-evidence/manifest.json and t0043-validation.json record accepted P03 five-cell/240-worker and P02 three-cell/144-worker comparisons: 320 measured, 48 warm-up and 16 diagnostic workers; zero accepted worker failures/retries/replacements/pooling. Pre/post audits pass 31,647/32,199 bindings; exact source/build/link/ABI/runtime/policy capsules and 559/353-file private evidence seals retained. Both arms pass full qualified geometry-manifest/persistent-object/payload/calibration checks and 91 native cases each per comparison; 20 Python cases pass normal/optimized. P03 primary hooks improve locally, whole-open CIs cross zero, RSS rises and Draft remains legacy FAIL; P02 primary saves improve and controls remain INCONCLUSIVE with zero-payload hooks prospectively N/A. Earlier 135-row verifier-scope attempt is explicitly ineligible, audited/preserved and never pooled; additive manifest repair independently CAD/performance reviewed. P08 has a source-reviewed current-effect attribution exclusion preserving T014/T017/T007, no speed claim, sole residual owner T004.4; B011 inactive. Final CAD/statistical/privacy reviews clear publication; historical receipts/authority snapshots and unrelated console work preserved. Same pair reused; debug untouched. Package lint/traceability pass. T004.4 is next; parent T004, SC-005 and T010 applicability remain open; no production acceptance.
    - Owner: local-developer
    - Resolved decisions: D001, D007
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.
    - Context evidence: D001 inventory remains the measurement baseline; D007 review-plan binding is required before new sampling. Accepted P03/P02 comparisons and the reviewed P08 exclusion are recorded in t0043-results.md/json; D007 workflow dispositions remain T004.4.

  - Status: Complete per-change measurement/disposition under prospectively frozen D007; original statistical labels preserved. T004.4 owns workflow cost-benefit/operating scope, including tails and process peaks; T010 final applicability.
  - [x] T004.4 Review retained optimizations' workflow cost-benefit and operating scope.
    - Depends on: T004.3
    - Requirements: Requirement 3 AC1, AC3, AC4, AC5, AC7, AC8; SC-005
    - Design: design.md#cost-benefit-acceptance-policy
    - Files: `acceptance-policy-v2.md/json`, `cost-benefit-review.md/json`,
      `d007-validation.json` (preserved), `t0044-cost-benefit-review.md/json`,
      `t0044-validation.json`, `t0044-prior-authority/`, `verification.md`,
      `performance-results.md`, `tasks.md`, `../../reference/document-open-performance.md`,
      `../../runbooks/freecad-performance-optimization.md`
    - Acceptance: Resolve each retained change's D007 trigger review with absolute benefit/tails,
      confidence, workflow frequency, transient/retained memory/RSS, materialized-image count,
      overlapping work, operating scope, rationale/mitigation and accountable owner. Use existing
      applicable evidence or reviewed accounting/equivalence; collect targeted new evidence only
      for real unanswered applicability questions, never solely to clear old numeric vetoes.
      Address P05 many-image memory and P07 first-use/tail experience; do not infer BIM host cause.
      A justified trade-off may be accepted without rewriting an algorithm to old numeric caps.
      Preserve original v1 outcomes and keep final-source/platform obligations with T010.
    - Validation: Independent review of the complete evidence and trigger-closure fields;
      benchmark only under a new predeclared plan when actual evidence gaps require it.
    - Evidence mode: artifact
    - Evidence: 2026-11-09: t0044-cost-benefit-review.md/json and t0044-validation.json resolve all eleven D007 fields for P01-P08 using existing qualified evidence and reviewed accounting. P01/P02/P03/P04/P05/P07 retained in explicit selected scopes; P06/P08 attribution exclusions adopted without independent speed claims. P05 <=3 materialized 4096-equivalent fallback planes/one conversion is a headroom-conditional accounting scope, not a tested count/software clamp; temporary/retained/RSS/GPU limits explicit. P07 full20pair first-show p95 1843.09/max1930.75ms accepted only for hidden/subset workflow; n=2 diagnostic remains separate. No BIM host-cause inference or v1 reclassification. Independent CAD, performance/governance and operations/docs reviews clear corrected final content. All26 immutable source bindings, eight prior-authority snapshots,88 fields, exact five-cell P05 tails, logical accounting and new links/privacy checked; scoped hooks exit0 but exclude docs. Durable reference/runbook promoted. No new source changes/builds/runtime samples. Final source/platform and SC-005 acceptance remain T010; closure T011.
    - Context evidence: D007 selects policy and implementation direction; final operating-scope decisions remain, not new algorithm work. T004.3 reports local P03 hook gains without whole-open confidence; Draft open-p95 legacy failure; P02 primary median/p95 gains but maximum 5054.700→5193.201 ms; process peaks about 1.7–2.1 MiB higher. Review those triggers plus unknown workflow frequency/concurrency under D007; no automatic rerun solely for old numeric vetoes.
    - Owner: local-developer
    - Resolved decisions: D007

  - Status: Complete scoped D007 workflow/operating-scope review. Parent T004 local review may close; next T010.1 confirms exact final source/build/platform applicability. No final production approval.
## Phase 4 — Prepare Delivery And Close

Exit: exact final-production evidence applicability, required broad/platform checks, durable
promotion and owned residual dispositions are complete.

- [x] T010 Verify final production revisions and prepare review evidence.
  - Depends on: T004
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `verification.md`, `../../reference/document-open-performance.md`,
    `t010-final-source-manifest.json`, `t010-build-receipt.json`, `t010-applicability.md/json`,
    `t010-results.md/json`, `t010-validation.json`, `t010-evidence/`, `t010-prior-authority/`
  - Acceptance: D003 confirms exact final source/dependency revisions and platform scope; all
    affected automated, archive, calibration/lifecycle, visual/manual and benchmark evidence is
    rerun or has reviewed equivalence. Build/focused/full-suite results and per-optimization timings
    satisfy mandatory hard gates and the owner-reviewed D007 workflow dispositions; no unresolved MoE acceptance blocker remains.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: t010-results.md/json; t010-validation.json; t010-final-source-manifest.json; t010-build-receipt.json; t010-evidence/manifest.json. All four children complete on exact v3 base-plus-five-file patch. Locked build, focused/regression/CAD/native/memory and unfiltered full CTest pass (2426 passed, zero failures, two existing skips, eight disabled). Independent CAD, performance/provenance and operations/document reviews complete; final hash-bound receipt retains original policies/results and archived prior authority. Linux-only scope selected by owner; final local acceptance does not certify historical numeric timing, general production/release or other platforms. T011 remains open.
  - Resolved decisions: D003 (initial candidate/policy); T010.1 confirms final v3 base-plus-patch Linux composition
  - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see validation-manifest.md
    and record actual execution provenance.
  - Completion-time context: Final source/build, full-suite and independent review receipts bind the exact uncommitted patch. Earlier GNOME/IBus Memcheck startup failures remain ineligible; final generic/Fusion+compose profile passes 26 Qt cases with zero access errors and only the existing libunwind probe suppression. Other profiles, leaks and undefined-value analysis remain unqualified.

  - Status: Complete bounded Linux final qualification; all T010 children complete. Next T011 promotion and whole-spec closure audit.
  - [x] T010.1 Confirm final production revisions and invalidate affected evidence.
    - Depends on: T004
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `../../reference/document-open-performance.md`,
    `t010-final-source-manifest.json`, `t010-build-receipt.json`, `t010-applicability.md/json`,
    `t010-results.md/json`, `t010-validation.json`, `t010-evidence/`, `t010-prior-authority/`
    - Acceptance: Reconfirm the T019 manifest against final production patches; enumerate changed
      source/dependency/build/fixture/environment inputs and invalidate/reconcile affected
      correctness, manual/visual, archive, lifecycle and benchmark results before sign-off.
    - Evidence mode: artifact
    - Evidence: 2026-11-09 v3 final source manifest selects root b5f1581921 plus exact five-file t010-evidence/final-corrections.patch and30 source/test/config hashes, same7clean dependencies/510packages/nine pinned fixtures. Adds independently reviewed App ObjectStatus::Restore guards/classifier and two red ready/late regressions; preview hide idempotence repairs traced close warnings; retains finite loft/Measure fixture fixes. Old image trace omitted App-only restore and is not complete zero-premature evidence. Prior source/failed and green full attempts retained. Exact Linux/compose/X11 softwareGL, generic/Fusion viewer/memory scope and process-level user-site masking selected; earlier contaminated native runs ineligible. Final fresh build,focused/full and source/runtime/manual/memory gates required; old timings remain historical measured data, not exact-final claims.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - Status: Complete final v3 base-plus-five-file-patch reconciliation; CAD source review cleared. Rebuild/focused/full on this composition before T010.4 final review.
  - [x] T010.2 Run pixi run build -j 2 and the affected focused suites on those revisions.
    - Depends on: T010.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `../../reference/document-open-performance.md`,
    `t010-final-source-manifest.json`, `t010-build-receipt.json`, `t010-applicability.md/json`,
    `t010-results.md/json`, `t010-validation.json`, `t010-evidence/`, `t010-prior-authority/`
    - Acceptance: Run pixi run build -j 2 and the affected focused suites on those revisions.
    - Evidence mode: artifact
    - Evidence: 2026-11-09 final v3 five-file patch build-qualified-final pixi locked build succeeds; t010-build-receipt.json pins30 source hashes,18 binaries/buildIDs/ldd closure, normal probesOFF/macros/exportsabsent. Fresh process-user-site-masked focus passes310App,26Reader,82bitmap,eight Qt image/render/tree/lifetime/selection/DocumentUpdates targets and dependent CAD. Both original full-gate failures pass; new App-only ready/late restore rows RED on oldguard and GREEN on corrected source. Full26Qt lifecycle Memcheck0errors/1existingnarrowlibunwind suppression in compose/generic/Fusion profile. Private isolated native stage run nowpasses correctfullimage/calibration0->1leaves2/repeat/post-close and zeroimage/Coinwarnings; actualmoduleclosure has0user-site libraries. Fullgate rerun on this exact composition remainsT010.3.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - Status: Complete final v3 build and focused/regression/memory qualification. T010.3 final masked full CTest next; final per-change/runtime/document evidence assembled in T010.4.
  - [x] T010.3 Run pixi run test and record any failure/skip with its acceptance impact.
    - Depends on: T010.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `../../reference/document-open-performance.md`,
    `t010-final-source-manifest.json`, `t010-build-receipt.json`, `t010-applicability.md/json`,
    `t010-results.md/json`, `t010-validation.json`, `t010-evidence/`, `t010-prior-authority/`
    - Acceptance: Run pixi run test and record any failure/skip with its acceptance impact.
    - Evidence mode: artifact
    - Evidence: 2026-11-09 final v3 exact five-file patch full CTest passes:2426passed,0failed,2existing unconditional backup skips,8pre-existing disabled tests of2436registered; JUnit+complete log bound in t010-evidence/final-full-ctest.*. Same repository test-task body executed after verified locked Pixi activation under user-site mask; nested pixi run --locked test innamespace stopped before tests dueenvironmentprobe failures, so documented lower-level diagnostic route runs exactctest --test-dir build/debug with output/JUnit flags only. No filters/exclusions/locksolve/fabricatedplatform used in accepted run. Earlier initial2failure and two-file green full runs remain historical; final30sourcehashes/510package identity/lock unchanged. Relevant newAppRestore/Coinclose/coreimage/tree/map/archive/Sketcher gates allpass. Two skips/eight disabled remain coverage limits,not passes.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - Status: Complete final v3 full graph on exact corrected source and closed validation runtime. Zero failures; T010.4 final independent review/evidence completion next.
  - [x] T010.4 Revalidate and assemble final performance, manual and platform evidence.
    - Depends on: T010.3
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
      Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `../../reference/document-open-performance.md`,
    `t010-final-source-manifest.json`, `t010-build-receipt.json`, `t010-applicability.md/json`,
    `t010-results.md/json`, `t010-validation.json`, `t010-evidence/`, `t010-prior-authority/`
    - Acceptance: Rerun affected measurements/manual checks or record reviewed equivalence tied to
      D003 final revisions; assemble per-optimization D001 statistical outcomes and separate D007 cost-benefit decisions,
      first-frame/show latency, memory effects, platform limits and focused commit boundaries. Old
      timing assembly alone is insufficient.
    - Evidence mode: artifact
    - Evidence: t010-results.md/json; t010-validation.json; t010-evidence/manifest.json; t010-final-source-manifest.json; t010-build-receipt.json. Final v3 five-file patch built and qualified: full CTest 2436 registered/2426 passed/0 failed/2 existing skips/8 disabled; focused and 26-case scoped Memcheck pass; native user-site-masked module closure, pixel/calibration/first-show and post-close checks pass. Three independent expert reviews cleared substantive content; final mechanical audit verified 169 hash bindings, 65 evidence files, 42 stable task IDs and unchanged 26 historical inputs. D007 local direction/first-use/logical-RAM acceptance only; historical P01/P07 numeric/runtime and old classifier limits explicit. User selected Linux-only; other platforms/profiles unqualified. T011 owns promotion/closure and routed-task tool reconciliation.
    - Candidate manifest: spec003-linux-x86_64-debug-candidate-20261106-v1; see
      validation-manifest.md and record actual execution provenance.

  - Status: Complete final source/runtime/document review for bounded Linux acceptance. No new timing campaign or release/cross-platform approval. Next T011.
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
