---
title: Performance optimization open decisions
doc_type: spec
artifact_type: open-decisions
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# Open Decisions

## Purpose

This ledger is authoritative for unresolved decision IDs/status/ownership. Design
and traceability explain their implementation impact. D004 selects the corrected native-probe campaign; all four decisions are resolved
as planning/execution selections. Runtime acceptance remains T020/T004/T010.
D001 is resolved by T001; D002 is resolved as an owned optional backlog route.
D003 is resolved for initial local candidate/provenance policy under the user
instruction to complete T019; final production confirmation remains T010.1.

## Decision Log

| ID | Decision Needed | Options | Owner | Needed By | Status |
| --- | --- | --- | --- | --- | --- |
| D004 | Corrected native-probe campaign after incomplete freeze-1 | Fix owned-window selection and first-swap capture; qualify and freeze all seven whole cells without threshold waiver | local-developer | T020 | resolved |

## Blocking Impact

| Decision ID | Blocks | Impact If Unresolved |
| --- | --- | --- |


## Resolved Decisions

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
