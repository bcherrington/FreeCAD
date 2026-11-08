---
title: Specification closure log
doc_type: history
status: active
owner: local-developer
last_reviewed: 2026-11-08
---

# Spec Closure Log

## Entries

### 2026-11-05 - 000-sketcher-diagnosis-performance

- **Spec:** `docs/specs/000-sketcher-diagnosis-performance/`
- **Title:** Sketcher diagnosis performance requirements
- **Final spec commit:** `c0615282aa`
- **Closure cleanup commit:** `191ba1e08f`
- **Closure action:** retained-as-history
- **Durable docs updated:**
  - `docs/reference/sketcher-performance-diagnostics.md`
  - `docs/runbooks/sketcher-performance-investigation.md`
  - `docs/backlog/README.md`
  - `docs/backlog/sketcher-performance.md`
- **Verification summary:** All T001–T007 complete; closure check ready with seven
  requirements covered, no blockers, lint zero errors/two existing authoring
  advisories. Source/test and independent docs/operations expert reviews found no
  blocking implementation defect; profile correction and optional prose applied.
  T007 validates 195 Markdown file/anchor targets, embedded Python/shell syntax, JSON, eight
  valid backlog items and CRLF-aware whitespace. No new build or GUI run for
  documentation-only promotion. Retained Linux evidence: native 149/149, affected
  Python 97 run/one existing skip, full model comparisons, 190 matched triples,
  bounded nine-stage GUI checks and 13-sketch updated-copy qualification.
  Indicative timing acceptance is satisfied; earlier strict-gate uncertainty is
  unchanged. Current reference/runbook/backlog supersede temporary spec guidance.
  Docs repository promotion commit: `9bc3677`; root final spec: `c0615282aa`.
  Docs repository archive commit: `8ba1733`; root archive: `191ba1e08f`.
  Cleanup hashes are resolved from actual commits; archive-index consistency
  has zero diagnostics and active inventory excludes this historical package.
- **Residual risks:**
  - Indicative speedups do not guarantee regression-free or zero-overhead behaviour.
  - No fresh full GUI build or Windows/macOS CI certification; bounded GUI checks
    used existing consumer binaries with current App code.
  - Intermediate observer/addon dependencies are not universally certified; B008
    owns additional external-property observer mutation coverage.
  - Exact immediate historical mutation precursors remain unverified (B006).
- **Follow-up:** local-developer owns proposed/deferred B001–B008 in
  [the backlog](../backlog/README.md). Public preparation additionally requires
  personal human review/testing and communication, issue alignment, fresh affected
  GUI/platform CI, a focused clean upstream base and AI disclosure in source
  commits. See [private delivery review](../reviews/sketcher-diagnosis-delivery-review.md).
  No integration merge, push, install, public submission or solver-default change.
  Feature changes remain on `perf/sketcher-diagnosis`; the earlier crash fix is
  already in integration base `36aa20c979`. The package is retained in place for
  historical evidence and the frozen `workloads.json` consumers; no current
  knowledge is left solely in the temporary package.

### 2026-11-08 - 004-sketcher-external-reference-reattachment

- **Spec:** `docs/specs/004-sketcher-external-reference-reattachment/`
- **Title:** Sketcher external reference reattachment
- **Final spec commit:** `96924c50a2`
- **Closure cleanup commit:** `c675dcde8c6ccaaae2b4710badf0ca46bdbb00af`
- **Closure action:** retained-as-history
- **Durable docs updated:**
  - `src/Mod/Sketcher/App/SketchObject.pyi`
  - `docs/reference/sketcher-external-reference-reattachment.md`
  - `docs/reviews/sketcher-reattachment-delivery-review.md`
  - `docs/backlog/README.md`
  - `docs/backlog/reattachment-delivery-follow-ups.md`
- **Verification summary:** Native/Python operation and Requirements 1–5 /
  CP-001–CP-005 locally implemented, qualified and promoted. Build passes; all
  163 native cases pass, including thirteen feature cases and the new solver
  failure/recovery regression. Sixteen public cases pass under each solver
  selection 0/1/2 (48 executions, no skips). Full TestSketcherApp runs 113 cases
  with zero failures/errors and one existing secant skip. Final full CTest lists
  2434 entries: 2422 pass, two fail, two runtime skip, eight disabled. No green
  whole-project claim. Exact manifests, source/build hashes and raw receipts
  are in `completion-validation.json` and completion-evidence/ within the package.
  Four private simulated specialist reviews found no remaining implementation
  blocker after corrections. Package/reference/review/backlog lint and scoped
  hooks/format/CRLF checks pass.
- **Source commits:** Native helper `454fe91954`; separate GCS status/fallback fix
  `42144bc1e9`; public binding, lifecycle tests and promotion `96924c50a2`.
  Docs implementation/promotion commit: `6bfa619`.
  Docs archive commit: `3f3df11206c9e264e4b871b6539ba4ae197c7a56`.
  Actual cleanup identifiers are reconciled after commit creation; no future
  commit identity is claimed.
- **Residual risks:** B012 owns the isolated Measurement handler-null test; B013
  owns the Part loft-selector test using Part line profiles. Their source modules
  were not changed for this API and their scenarios do not invoke Sketcher.
  B014 owns human/upstream API review, clean-base/platform CI and separate-worktree
  PR preparation. Local reviews are not actual maintainer approval.
- **Follow-up:** local-developer owns B012–B014 in
  [the durable follow-up record](../backlog/reattachment-delivery-follow-ups.md).
  The shared branch and other agents' work are preserved. No install, public
  submission or live operator CAD/MCP delivery is claimed. The package is
  retained in place for reproducible evidence; current knowledge is promoted.
