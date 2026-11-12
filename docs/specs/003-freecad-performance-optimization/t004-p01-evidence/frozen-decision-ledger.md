---
title: Performance optimization open decisions
doc_type: spec
artifact_type: open-decisions
status: draft
owner: local-developer
last_reviewed: 2026-11-08
---

# Open Decisions

## Purpose

This ledger is authoritative for unresolved decision IDs/status/ownership. Design
and traceability explain their implementation impact. D001–D006 are resolved
planning/execution selections. D005 resolves retention and the technical validation
plan, not P07 acceptance. T004/T010 remain incomplete while retained changes fail
D001. T010.1 still confirms final production composition and evidence applicability.

## Decision Log

| ID | Decision Needed | Options | Owner | Needed By | Status |
| --- | --- | --- | --- | --- | --- |

No open decisions. See the resolved D005 plan below.

## Blocking Impact

No unresolved decision IDs remain. P07's failed/inconclusive acceptance evidence
still blocks T004 completion and production acceptance. A new material behavior,
threshold or measurement-profile choice requires a new dated decision before
implementation or sampling; resolved D005 does not waive D001.

## Resolved Decisions

### D006 — P01 Structurally Empty Payload Controls

- **Status:** resolved on 2026-11-08 for the new P01 study before any samples.
- **Owner:** local-developer; delegated technical selection under the instruction
  to continue T004.1, with independent `p01_instrumentation_review` review.
- **Evidence:** [public archive audit](t004-source-review/p01-public-payload-coverage.json)
  matches the unchanged D001 fixture hashes. EngineBlock and PartDesignExample
  declare no included-file properties. BIMExample has one nonempty 519-byte
  payload; ArchDetail has one nonempty 5487-byte payload. Empty declarations are
  not executed payload hooks. Runtime qualification must confirm these counts.
- **Chosen disposition:** keep all five P01 cells and unchanged samples/endpoints/
  thresholds. For EngineBlock and PartDesignExample only, classify
  `restore_payload_ms` as **not_applicable**, never a passing restore measurement.
  Freeze the exact fixture hashes and D006 disposition in each control context.
- **Required evidence:** those two controls must have exactly zero included
  properties/bytes, completed/failed/reused/active hooks, native duration and
  restore metric. Their full model/reference, document-open, RSS and logical
  image-resource checks still run with all original thresholds. Unexpected
  nonzero hooks or changed coverage rejects the record. The private primary,
  BIMExample and ArchDetail require positive qualified payload coverage.
- **Rationale:** an unexercised hook cannot establish payload performance. This
  explicit structural inapplicability preserves useful non-payload controls and
  follows D001's zero-coverage decision requirement without dropping a fixture,
  inventing timings, changing a budget or waiving safety. P07 is unaffected.
- **Implementation:** the assessor binds every P01 duration to native counters,
  rejects undeclared zero coverage, and recognizes only this exact two-fixture
  D006 route. Synthetic tests distinguish not_applicable from a payload pass.

### D005 — Retain P07 And Select Bounded Validation Plan

- **Status:** resolved on 2026-11-08 for technical planning only.
- **Owner:** local-developer. The user selected retention and requested T004;
  Codex records the technical plan under that instruction, with independent agent
  review. This is not human/maintainer approval of a performance tradeoff.
- **Chosen option:** retain full-resolution lazy P07 and adopt
  [t004-execution-plan.md](t004-execution-plan.md). Keep D001's existing first-show
  500/1000 ms and repeat 50/100 ms budgets and all other rules unchanged. No exact
  replacement criterion or production behavior change is selected.
- **Rationale:** preserve the owner's opening/RAM benefit direction while
  investigating first-use cost and noisy controls. Both T020 campaigns keep their
  failed/inconclusive outcomes; P07 remains **NOT ACCEPTED**.
- **Source:** construction anchor `613def1ab6bf615f82fe163e3b41489d0228acbf`;
  all eight affected source files match committed T020. New common instrumentation
  and actual arm/build/configuration/fixture identities must be frozen before
  execution. P07 stays identical across non-P07 arms; concurrent Sketcher work is
  excluded. Final integrated applicability remains T010.
- **Diagnostic boundary:** at most 16 fresh GUI workers/20 minutes, fixed AB/BA
  across four contexts, explicitly A=eager/B=lazy with the sole reviewed visibility
  delta. Separate decode/conversion/native completion/event wait and observe host
  load; these exploratory records are ineligible for acceptance. No retries or
  extra pairs. The plan defines safety/ownership stop conditions and host controls.
- **Re-entry:** a concrete source, probe or measurement-condition correction and
  new dated decision/freeze are required before a fresh complete seven-cell P07
  campaign. Requalify affected fidelity, lifecycle/calibration, tree, frame/context
  and trace checks. No old-run retry, pooling, outlier deletion or threshold waiver.
- **Execution effect:** T022 planning may complete; T004.1 may construct and qualify
  P01 with shared safety and lazy behavior. Its draft scalar source is reviewed,
  not built or runtime-qualified. Five P01 cells need a new probe, matched builds,
  both-arm T002/T018 checks and exact execution freeze. T004/T004.2/T010 cannot
  complete while applicable P07 gates remain failed.
- **Review:** `t004_plan_review` inspected the actual plan, source/delta/fixture
  audit and draft patch. Missing header identity and diagnostic arm definition
  were corrected; final review reports no remaining blocker.
- **Evidence:** [static audit](t004-comparison-readiness.json),
  [P01 draft receipt](t004-source-review/p01-arm-construction.json), immutable
  [T020 outcomes](performance-recovery-2-results.md). Current task state is in
  tasks.md; T020 machine receipts retain their historical completion-time route.

### D001 — Predeclare comparative benchmark protocol

- **Status:** resolved on 2026-11-06 for Phase 1 planning.
- **Owner:** local-developer; recorded by Codex under the user's instruction to
  complete T001, as delegated technical selection, not independent human or
  maintainer approval.
- **Chosen option:** [benchmark-protocol.md](benchmark-protocol.md) and
  [benchmark-plan.json](benchmark-plan.json), `spec003-benchmark-20261106-v1`.
  Eight exact optimization origin pairs, per-change primary/control fixture
  identities and recipes, matched RelWithDebInfo arms, 20 paired measured runs,
  fixed warm-ups/order/cache policy, bootstrap/variability rules, improvement,
  non-regression, first-show/repeat-show and resource budgets are predeclared.
- **Rationale:** separate each retained optimization's evidence and detect work
  shifted to first display or larger decoded/Coin storage. Historical unsafe
  trees are lineage only; both runtime arms must preserve Phase 2 fixes.
- **Execution boundary:** exact corrected arm commits/build/probe/generated-output
  receipts are frozen before Phase 3 samples using the fixed construction rule.
  T020/T004 implement probes/assessment and collect results; T010 confirms final
  applicability. No runtime acceptance or performance result is approved here.
- **Change rule:** failed/noisy/missing coverage stays failed/inconclusive/blocked.
  Changed thresholds, fixtures or isolation require a dated owner decision and
  new samples; retain prior evidence. Do not waive safety or reinterpret results.

D001 execution clarification, 2026-11-07: the pre-sample
[T020 measurement review](t020-measurement-review.md) records the independently
reviewed P06 subsumption/exclusion and P07 arm-specific cache expectations. Numeric
thresholds, fixtures and sampling rules were unchanged; at that pre-sample
clarification no acceptance samples had been collected. Subsequent qualification
and the incomplete first collection are recorded in performance-results.md and D004.

D001 execution clarification, 2026-11-08: the reviewed
[P01 measurement construction](t004-p01-measurement-review.md) completes the
console endpoint with at most one required recompute (frozen counts), normalizes
a shared generated SQL parser, and applies D006's two structural-zero payload
metrics. All five cells, original sample counts and numeric thresholds remain.
No P01 acceptance sample has been collected at this clarification checkpoint.

### D002 — Preserve Current Full Resolution; Route Exploration

- **Status:** resolved as scope/routing on 2026-11-06 under the user's instruction
  to replan execution. Owner: local-developer; recorded by Codex as a delegated
  planning action, not independent maintainer/human runtime review.
- **Chosen option:** retain original/full-resolution semantics in active spec 003.
  Defer the optional reduced-resolution behavior proposal to
  [B009](../../backlog/spec003-follow-ups.md#b009-reduced-resolution-texture-exploration).
- **Rationale:** current calibration, safety and latency/memory acceptance must
  complete without adding another visual/physical behavior change to the slice.
- **Boundary:** no prototype, fidelity tradeoff or lower-resolution display is
  implemented or approved. B009 needs new evidence and a scoped proposal before
  activation. T009 records this disposition; T011 audits ongoing ownership.


### D003 — Initial local candidate and applicability policy

- **Status:** resolved for initial selection; exact final production revisions
  still require T010.1 confirmation and affected-evidence reconciliation.
- **Owner:** local-developer.
- **Resolved:** 2026-11-06, executed by Codex under the user's instruction to
  complete T019. This records delegated technical selection, not independent
  human or maintainer approval.
- **Chosen option:** [validation-manifest.md](validation-manifest.md), manifest
  `spec003-linux-x86_64-debug-candidate-20261106-v1`. Root `f7fe2520c2edf25a17f3ad1fde084116cb1627d4`, seven matching clean recursive Git dependencies,
  locked Linux x86_64 Debug candidate, recorded package/toolchain/fixture
  identities. Eight pre-existing binaries/libraries remain historical and
  ineligible until tied to a new build/execution receipt.
- **Rationale:** name the actual local integrated composition before safety
  corrections and acceptance. Source/submodule/config cleanliness was inspected;
  direct metadata resolves the stale Workbench composition limitation. This is
  not selection of a clean upstream PR base or a universal performance baseline.
- **Change rule:** source/build/dependency/fixture/environment changes rerun
  affected automated/manual/archive/calibration/lifecycle/benchmark checks or
  have explicit reviewed equivalence. Record actual new source/binary receipts;
  final cherry-pick/rebase selection is checked at T010.1.
- **Remaining execution:** fresh builds/runtime receipts, source fixes and
  platform gates; D001 protocol is resolved separately above. The later D002 routing decision is
  recorded above; this does not rewrite the immutable T019 intake snapshot.

### D004 — Corrected T020 Native-Probe Campaign

- **Status:** resolved on 2026-11-07 as technical execution selection; runtime
  acceptance is not resolved by this decision.
- **Owner:** local-developer; recorded by Codex under the instruction to continue
  T020, as delegated technical selection. Independent agent review is not human
  or maintainer runtime approval.
- **Chosen option:** [t020-recovery-2-plan.md](t020-recovery-2-plan.md), campaign
  `recovery-2-corrected-native-probe`. Correct the common Python probe's
  focus-dependent MDI selection and late post-event-dispatch frame capture;
  retain the accepted first qualifying native swap with explicit GL-state/context
  restoration. Use separately frozen post-failure diagnostics for all seven cells.
- **Rationale:** diagnostics measured 6.355–10.789 ms of post-swap tail and
  reproduced `No active document subwindow`. The corrected worker passes all
  fourteen frozen-context qualifications, six ownership controls and 551 focused
  C++ tests per arm. This does not classify the lost original failure or explain
  the first campaign's larger timing variability. Both remain recorded.
- **Exact arms:** lazy/common `613def1ab6bf615f82fe163e3b41489d0228acbf`;
  eager baseline `c574d418c015972cbe4109983c289ce963d24b47`. Each arm's full `src`
  tree and native outputs are identical to its qualified origin; copied regular
  outputs have independent inodes. No new C++ compilation is claimed.
- **Qualification receipt SHA-256:** `a1ad40bbe6bc83c8761ef7776f08c93cdb44c406eb1d04e665e5c8579b2e49e9`.
  The protected recovery directory is disjoint from freeze-1 and contains fresh
  contexts, current post-qualification host/environment receipts, source/build
  equivalence and ownership-control evidence. Final concrete-freeze review and
  before-collection audit must pass before sampling.
- **Sampling boundary:** all seven cells start at pair zero with exactly one
  attempt per cell, one diagnostic pair, three warmup pairs and twenty measured
  alternating pairs. No retries, resumed/combined partials, sample pooling,
  outlier selection, threshold relaxation or selection between campaigns.
  Any new failure or excessive variability keeps its declared outcome.
- **Retained first-campaign disposition:** original hidden primary remains
  aborted/ineligible/unassessed; all-visible and PartDesign timing remain
  inconclusive; completed control/auxiliary results remain bound to their old
  freeze. None becomes a pass through this new selection.
- **Unchanged contract:** D001 numeric rules, fixture identities, profile/cache
  policy, full resolution and Phase 2 safety remain required. Fixed capture-buffer
  driver effects remain in process RSS; logical image counters exclude that
  harness storage. T004/T010 stay gated by actual T020 acceptance.

Resolve future decisions only with owner, date, chosen option, rationale and
artifact/task updates. Preserve stable IDs and historical provenance.

## Discovery Guardrail

Query spec_open_decisions and compare IDs/statuses with this ledger. Missing or
conflicting discovery is a handoff blocker; inspect this file and design directly
rather than interpreting an empty result as approval. Advisory context readiness
means usable context, not resolved decisions or satisfied task dependencies.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md#open-questions)
- [Tasks](tasks.md)
- [Traceability](traceability.md#open-decision-impact)
- [Verification](verification.md#moe-review-findings)
