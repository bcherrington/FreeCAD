---
title: Sketcher diagnosis local delivery review
doc_type: history
status: active
owner: local-developer
last_reviewed: 2026-11-05
---

# FreeCAD Maintainer Pre-Review

**Verdict:** NOT READY for public upstream submission; local T007 delivery passes.
**Risk:** Medium.
**Base reviewed:** `36aa20c979..dfd398f31c` plus T007 documentation promotion.
**Scope:** private AI-assisted source/test review and independent documentation/
operations review. This is neither human approval nor a FreeCAD maintainer decision.

## Findings

- **[BLOCKER][POLICY] Public submission only:** [AI_POLICY.md](../../AI_POLICY.md),
  [CONTRIBUTING.md](../../CONTRIBUTING.md) and the
  [PR template](../../.github/pull_request_template.md) require contributor
  understanding, personal review/testing, disclosure and human-authored public
  communication. Source commits `9bc0eb91ff` and `77ca0b6d94` lack the applicable
  AI-assistance trailer. Before public submission the contributor must personally
  review/test and prepare disclosed logical commits and communication. No public
  submission or history rewrite is performed here. New local delivery commits
  include an assistance trailer; that does not certify human review of the source.
- **[REQUEST][TEST] Nonblocking local coverage follow-up:** the external-property
  refresh branches in [SketchObjectConstraints.cpp](../../src/Mod/Sketcher/App/SketchObjectConstraints.cpp)
  cover observer edits to `ExternalGeo` / `ExternalGeometry`. Existing
  [observer tests](../../tests/src/Mod/Sketcher/App/SketchObjectRecompute.cpp)
  exercise Geometry/Constraints edits; the external-index test does not edit
  during diagnostic notification. No defect was observed. Add a focused normal-
  notification external edit regression before upstream preparation; owned
  [B008](../backlog/sketcher-performance.md#b008-external-observer-regression)
  records the exact comparison and exit evidence.
- **[REQUEST][TEST] Public preparation:** current App/native/Python/model evidence
  and bounded GUI checks do not establish a fresh full GUI build or Windows/macOS
  compatibility. The larger GUI rebuild did not complete; GUI checks used existing
  consumer binaries with current App code. Run the appropriate fresh affected
  builds and upstream CI before making broader compatibility claims.
- **[REQUEST][POLICY] Public preparation:** no agreed upstream issue/design
  alignment was established by this local delivery. Prepare a focused source PR
  against an agreed clean upstream base and check issue alignment. The feature
  branch includes local integration ancestry; do not submit that ancestry wholesale.
- **[REQUEST][PATTERN] Fixed in T007:** documentation review found the native test
  command reused `/tmp/sketcher-native`. It now allocates a unique profile root
  with `mktemp`, preventing stale configuration from contaminating validation.
- **[SUGGESTION][PATTERN] Applied:** the durable reference now states the current
  indicative timing policy directly; user-decision provenance stays in history.

The source reviewer found no blocking correctness, security or data-integrity
issue in the bounded implementation: one-shot context ownership, input refresh,
exception cleanup, disabled trace work, worker lifetime and unchanged async/
deferred/wait-only semantics were examined. This conclusion is scoped to reviewed
code and retained evidence, not universal addon or platform certification.

## Validation Performed

Two independent read-only reviews examined production changes and regression
tests, and durable prose/commands against actual trace schema and source. The
operations review checked exact environment control versus Log sink, copied input,
separate profiles, privacy, interval unions, counter stages, limits and backlog
routing. Both returned no blocking implementation defect. The requested command
correction and optional prose improvement were applied.

T007 validates Markdown destinations, embedded Python syntax, shell syntax,
backlog schema, spec traceability/lint, CRLF-aware whitespace and unchanged source/
test/profile code relative to `dfd398f31c`. No new native build, benchmark or GUI
session is run for this documentation-only change. Earlier Linux checks remain
retained evidence: native 149/149, affected Python 97 run with one existing skip,
model comparisons, 190 matched timing triples and nine bounded GUI stages. See
[verification](../specs/000-sketcher-diagnosis-performance/verification.md).

## Process / Working-Group Flags

Issue/design alignment remains a public-preparation action for local-developer.
No new public modelling API or user interaction is introduced here. No separate
FEP or working-group approval requirement was established in this review. Future
numerical/default/cache changes require their own bounded specification and review.

## PR-Readiness Checklist

- Local-developer: personally review/test code and author public communication.
- Local-developer: establish issue alignment and a clean focused upstream base.
- Local-developer: complete fresh affected GUI/build/platform validation and B008.
- Local-developer: ensure appropriate AI disclosure in source commits and PR.

## Residual Risks And Integration

Performance evidence indicates improvement, not guaranteed speedup or absence of
regression/overhead. Strict historical gates remain inconclusive. Arbitrary addons
may depend on intermediate observer snapshots. Exact immediate mutation precursors
for historical long stalls remain unverified and owned by B006. No production
solver defaults, tolerances, async policy or operator models changed in T007.

All implementation remains on `perf/sketcher-diagnosis`; no merge, push, install,
public PR or release is performed. The prior crash fix is already in integration
base `36aa20c979`. T003 tracing `9bc0eb91ff` and T004 deferral `77ca0b6d94` remain
feature-branch changes. Local specification acceptance and historical closure do
not assert upstream readiness or authorize a new integration merge.

Current knowledge lives in the [reference](../reference/sketcher-performance-diagnostics.md),
[runbook](../runbooks/sketcher-performance-investigation.md) and
[owned backlog](../backlog/README.md). Historical evidence remains in place because
profiling helpers depend on its frozen manifest path; final and cleanup commit
identities are recorded in the [closure log](../history/spec-closure-log.md).
