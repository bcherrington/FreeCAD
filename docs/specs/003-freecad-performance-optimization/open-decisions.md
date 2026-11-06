---
title: Performance optimization open decisions
doc_type: spec
artifact_type: open-decisions
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Open Decisions

## Purpose

This ledger is authoritative for unresolved decision IDs/status/ownership. Design
and traceability explain their implementation impact. No active decision remains open.
D001 is resolved by T001; D002 is resolved as an owned optional backlog route.
D003 is resolved for initial local candidate/provenance policy under the user
instruction to complete T019; final production confirmation remains T010.1.

## Decision Log

| ID | Decision Needed | Options | Owner | Needed By | Status |
| --- | --- | --- | --- | --- | --- |

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
