---
title: Assembly motion suspension verification
doc_type: spec
artifact_type: verification
status: archived
owner: local-developer
last_reviewed: 2026-11-12
---

# Verification

## Baseline and identity

Source base `3a53ef2ce7`; implementation `f0dd422402`; tests `a73012a8cd` on
`feature/assembly-motion-suspension`. Docs are in a separate worktree/branch; existing dirty
docs are untouched. Agent Workbench had degraded indexing; claims use direct source and
executed checks. [Machine-readable identities/results](evidence/validation-results.json)
include the AssemblyApp SHA256, source hashes and toolchain. FreeCAD reports 27.1.0dev /
Dev Build; that version alone does not establish API availability.

## Quality Gates

| Gate | Result | Evidence |
| --- | --- | --- |
| Native/App lifetime and guarded motion | PASS: 10/10 native tests | evidence/native-tests.log |
| Public Python/API, callbacks, generated remap and outside behavior | PASS: 39/39 core tests, no skips | evidence/python-core-tests.log |
| Native instrumentation | PASS: three test cases; exercised guarded and restore phases; positive controls hit | evidence/native-motion-probe.log |
| Build | PASS: normal pixi build and focused test/script targets | evidence/build-summary.txt |
| Formatting/whitespace | PASS: applicable hooks and git diff --check | evidence/pre-commit.log; source commits |
| Broad Assembly workbench | 59/61 pass; E001 baseline exception | evidence/python-workbench-tests.log |
| Independent MoE review | All concrete blockers fixed and rechecked | review.md; promoted private review |

## Evidence Log

- `pixi run build -j 2`: exit 0. Initial in-progress test compile SIGBUS did not reproduce;
  configured syntax validation and subsequent builds passed. A later binding compile error
  (hidden isValid method) was corrected by qualifying the base validity check; final build passes.
- `FREECAD_USER_HOME=/tmp/freecad-motion-gdb-profile pixi run ctest --test-dir build/debug -R '^AssemblyObjectTest\.' --output-on-failure`:
  10/10 pass. System CTest could not read the newer discovery command; pixi CTest is required.
- `xvfb-run -a env FREECAD_USER_HOME=/tmp/freecad-motion-core-profile pixi run bash /tmp/freecad-motion-run-gui.sh --run-test AssemblyTests.TestCore`:
  39/39 pass, including 16 added cases. The durable `run-gui-tests.sh` is an exact copy of the
  executed wrapper. Native binary is the same build output. Existing pixi freecad wrapper drops
  extra CLI arguments, so the test wrapper forwards them explicitly and sets the same library paths.
- Same isolated GUI command with `--run-test TestAssemblyWorkbench`: 61 run, 59 pass, two
  unchanged view-mock errors. No new failing motion test remains.
- Isolated commands selecting the two view tests reproduce both errors. `git diff 3a53ef2ce7 --`
  on CommandCreateView.py, TestCommandCreateView.py and UtilsAssembly.py is empty. Failure is
  `_Move` mock lacking Label, independent of changed assembly motion calls. E001 excepts these
  existing fixtures for local feature acceptance; [B015](../../backlog/assembly-motion-suspension-follow-ups.md#b015-view-test-fixtures)
  owns correction separately. This does not claim the entire workbench suite is green.
- `gdb -batch -x native-motion-probe.gdb --args build/debug/tests/Assembly_tests_run` with the
  three filters recorded in the probe log: exit 0, all three cases pass. Guarded phase:
  zero native preparation, normalization or placement writes. Restore phase: zero native
  preparation/normalization and zero Assembly writer stacks; 17 structural placement writes
  initialize restored objects. Positive control: four solves, five normalizations, 164 placement
  writes, including eight Assembly writer stacks. Phase visit counts guard against vacuous pass.
- Before source fix f0dd422402, evidence/restore-bypass-before-fix.log records one runSolve and normalization because
  Document::restore sends fake Delete/New signals. Preserved evidence/restore-bypass-before-fix.log is historical,
  superseded by the passing final log. Registry now distinguishes real close from content reset.
- Applicable pre-commit checks pass on all 13 changed source/test files; commit hooks pass.
  Full-repository pre-commit and unfiltered project CTest were not rerun for this Assembly slice.

## Residual Risks

Windows/macOS CI, manual GUI drag qualification, upstream maintainer acceptance and independent
MCP receipt/recovery qualification are not established. Origin/root datums are supported grounds,
not endpoint roots in the conservative explicit admission API. Formula ownership/type validation
is not simulation expression evaluation. Some existing programmatic GUI fixtures print proxy
initialization tracebacks despite passing assertions; no claim of globally clean GUI console.
No unrelated mock repair, integration merge, installation, push or public submission was performed.

## Scope Reconciliation Before Closure

Requirements 1–3 and CP-001–CP-004 are complete within the finite supported native graph and
motion-control scope. Requirement 4 has no new regression: E001 is a bounded exception for
existing view mocks, owned by B015, not permission to ignore feature failures. B016 owns
platform/upstream/human and MCP integration qualification. Logs retain semantic output;
trailing whitespace was normalized for Git. Complete local evidence was committed as docs
4b6a821; durable API/reference and private review supersede delivery-scaffold prose.

Lifecycle checks before retention: package lint0 errors/0 warnings, evidence-quality13 concrete
records/0 warnings, closure check no blockers. Historical package retained in place; current
truth is the promoted reference/review and source API declarations.
