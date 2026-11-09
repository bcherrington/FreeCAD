---
title: Reattachment delivery follow-ups
doc_type: reference
artifact_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-09
---

# Reattachment Delivery Follow-Ups

The [backlog index](README.md) owns IDs, status and priority. These are inherited
whole-project test failures and external delivery work, not unfinished reattachment
implementation. Owner: local-developer. Exact CTest evidence is retained with
spec 004. No upstream issue was filed and no maintainer approval is inferred.

## B012 Isolated Measurement Handler Registration

- Status: completed locally in T010 commit `21903ca31f`; priority PR prep.
- Historical evidence (spec004): `MeasureElementType.testDatumAreaHandlerReturnsInvalidResult` fails at
  `tests/src/Mod/Measure/App/MeasureElementType.cpp:146`: the `Part` area geometry
  handler is null before the measurement is attempted. It reproduces in both the
  initial and final owned-profile full CTest runs.
- Scope separation: this test constructs a Part datum plane and accesses Measure's
  handler registry; it does not call Sketcher reattachment or the constraint solver.
  No Measure source/test was changed for spec 004. The registration root cause is
  not claimed diagnosed by this observation.
- Next action: inspect module/handler initialization in isolated GoogleTest
  discovery versus a whole-binary run; fix the owning fixture/registration in a
  separate change and rerun the isolated case and Measure suite.
- Exit: an initialized handler and correct invalid-result assertion in isolated
  CTest, or an explicit owning-test correction with regression evidence. Until
  resolved, the repository-wide run must not be called fully passing.

Resolution: the test fixture imports the real Measure module after application
initialization. The original invalid-result assertion passes in isolation and in
the final unfiltered Linux full graph. [T010 evidence](../specs/003-freecad-performance-optimization/t010-results.md)
binds source, runtime and results. The earlier next action/exit above is historical.

## B013 Part Loft Task Selector Test

- Status: completed locally in T010 commit `21903ca31f`; priority PR prep.
- Historical evidence (spec004): `TaskInlineAcceptProgressTest_Tests_run` fails in
  `loftAcceptUsesInlineProgressInsteadOfModalDialog`: BlockingLoftProfile and
  LoftProfile are absent from the available-object selector; acceptance reports
  that at least two profiles are required and the dialog remains open.
- Scope separation: `tests/src/Mod/Part/Gui/TaskInlineAcceptProgressTest.cpp`
  creates `Part::BlockingLineTest` and `Part::Line` profiles, not Sketcher objects.
  No corresponding Part GUI implementation or test was changed for spec 004.
  The final isolated-profile full run reports a test failure, not a passing GUI
  result; initial inherited-profile GUI aborts are not final qualification.
- Next action: investigate eligible-profile filtering and event/selector setup in
  the owning Part task test. Preserve unrelated GUI/performance work; do not
  weaken the inline-progress expectation to obtain a green run.
- Exit: the profiles can be selected and the existing loft acceptance test passes,
  followed by the affected Part GUI suite and repository-wide validation.

Resolution: finite single-edge profiles are admitted by the Part loft selector,
preserving closed-wire/single-vertex logic. The original selector/inline-progress
test passes in isolation and the final Linux full graph. The earlier next action/exit
above is historical. Other-platform and clean-upstream qualification remains B014.

## B014 Upstream Reattachment Delivery

- Status: proposed; priority PR prep.
- The local implementation and spec are complete. Prepare the focused upstream
  code/durable-document change in a separate worktree; preserve the shared branch
  and include the independently justified solver status fix as its own logical
  commit. Reconcile against the selected upstream base rather than transferring
  unrelated branch work or historical spec evidence indiscriminately.
- Human owner must review/test the change, establish issue/API discussion alignment,
  obtain Windows/macOS and clean-base CI, and resolve/disposition B012/B013 in the
  appropriate repository context. Check current AI_POLICY.md and rewrite public
  communication/commit messages personally; retain assistance disclosure.
- Exit: a human-reviewed, focused and CI-qualified PR or an explicit decision not
  to submit. Local simulated specialist reviews do not satisfy upstream approval.
