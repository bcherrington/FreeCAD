---
title: FreeCAD performance optimization requirements canonical context
doc_type: spec
artifact_type: canonical-context
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Canonical Context

## Purpose

Prevent historical intended behavior and dated local measurements from becoming
current implementation claims. This package was migrated from unnumbered legacy
artifacts on 2026-11-06. Its current acceptance remains draft.

## Authority Hierarchy

User/session instructions and root/deeper AGENTS.md govern execution. Repository
templates govern artifact shape. Source/tests establish implementation; this
package defines proposed acceptance and working design, not upstream approval.
Treat source/spec conflicts as reconciliation inputs. Neither spec nor imported
historical notes override code, tests, policy or live evidence.

## Always-Canonical External Sources

| Source | Authority reason | Handling |
| --- | --- | --- |
| `AGENTS.md` | Repository instructions | Read before implementation |
| `docs/templates/spec-package/README.md` | Current package convention | Preserve artifact roles and evidence limits |
| Source and tests in requirements.md baseline | Implementation truth | Recheck at the selected task revision |

## Spec-Canonical Working Sources

| Source | Role | Scope | Notes |
| --- | --- | --- | --- |
| requirements.md | Proposed acceptance | This package | Legacy FR/PR/SC aliases retained |
| design.md | Current working approach | This package | Source/spec conflicts remain explicit until verified |
| open-decisions.md | Authoritative decision status/ownership | D001–D003 | D001 protocol, D002 routing and D003 initial policy resolved; no runtime/final PR acceptance implied |
| validation-manifest.md and validation-manifest.json | Candidate identity and evidence applicability | T019 initial local Linux candidate | Historical binaries ineligible; actual new-build/runtime receipt required |
| benchmark-protocol.md; benchmark-plan.json; benchmark-runner.py | Predeclared measurement contract and bounded collector | T001/D001 | No FreeCAD probe, generated fixture, qualified arm or performance result claimed |
| tasks.md | Four-phase delivery queue and explicit next action | This package | T012 next; Phase 1 complete; phase order and dependencies govern execution |
| baseline-status.md | Existing implementation and dated evidence | Baseline inventory only | Source presence does not set execution status |
| traceability.md | Bidirectional context | This package | Partial coverage blocks closure |
| verification.md | Revision/date-bound evidence | Named checks only | No blanket readiness inferred |

## Imported Sources

| Spec path | Source path | Source revision or date | Status | Canonical scope | Promotion target |
| --- | --- | --- | --- | --- | --- |
| requirements.md; design.md | legacy-spec.md | Existing 2026-11-04 working document | adapted | Proposed behavior only | See requirements.md durable impact |
| verification.md | Pre-migration verification report | 2026-11-04 at 13d601b9dd6b66026e2d0dbb5c16156f18bdb592 | background | Historical results only | Current validated results to proposed runbook |
| design.md; verification.md | Current source paths in requirements.md | d0644236e41f35a849b12a1e77941aa88354496f | summarized | Presence of inspected implementation only | See requirements.md durable impact |

## MoE Evidence And Execution Boundary

Static MoE findings at root c69560af8a/docs 8d1abe9 are recorded in
verification.md#moe-review-findings. They establish source conflicts/risk and
missing acceptance contracts, not executed failures or completed corrections.
The earlier implementation-presence baseline does not override F001–F009.
Use the T019 manifest ID and actual new source/build/fixture execution receipts
before accepting source/test evidence;
reconcile exact final revisions and invalidated evidence at T010. Context-ready
packets never override review-needed decisions, prerequisites or safety blockers.

## Non-Canonical Background Sources

Legacy-spec.md and any retained plan.md, legacy-tasks.md and research.md are
historical migration inputs. Their checkboxes, transient paths, removed trace
switches and future-tense instructions do not authorize current work or prove
acceptance. No external document is copied into governing policy here.

## Replanned Execution Boundary

The user-authorized execution replan preserves stable IDs, completed T019/T021
artifacts and the immutable manifest. Five old partial task states are pending
remaining qualification; their imported observations are retained in the
baseline inventory. T001 resolves D001 with a predeclared protocol; Phase 2
starts with T012. Performance sampling still requires Phase 2 qualification. D002's B009 routing and B010/B011 optional investigations do not
approve new behavior or waive any current MoE safety/measurement requirement.
The original manifest's "remaining decisions" prose records its intake time;
current status is always the active decision ledger.

## Promotion Map

Accepted behavior/design goes to the explicit durable targets in requirements.md;
operational checks go to its runbook target. Local-developer owns routing
unfinished work to docs/backlog/README.md before closure. Historical evidence
stays dated or is retained with an explicit audit-only role.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Open decisions](open-decisions.md)
- [Candidate validation manifest](validation-manifest.md)
- [Baseline implementation inventory](baseline-status.md)
