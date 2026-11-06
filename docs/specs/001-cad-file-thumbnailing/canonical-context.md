---
title: CAD and mesh file thumbnailing requirements canonical context
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
| design.md | Current working approach | This package | Unresolved choices stay explicit |
| tasks.md | Delivery queue | This package | Read with requirements and design |
| traceability.md | Bidirectional context | This package | Partial coverage blocks closure |
| verification.md | Revision/date-bound evidence | Named checks only | No blanket readiness inferred |

## Imported Sources

| Spec path | Source path | Source revision or date | Status | Canonical scope | Promotion target |
| --- | --- | --- | --- | --- | --- |
| requirements.md; design.md | legacy-spec.md | Existing 2026-11-04 working document | adapted | Proposed behavior only | See requirements.md durable impact |
| verification.md | Pre-migration verification report | 2026-11-04 at 13d601b9dd6b66026e2d0dbb5c16156f18bdb592 | background | Historical results only | Current validated results to proposed runbook |
| design.md; verification.md | Current source paths in requirements.md | d0644236e41f35a849b12a1e77941aa88354496f | summarized | Presence of inspected implementation only | See requirements.md durable impact |

## Non-Canonical Background Sources

Legacy-spec.md and any retained plan.md, legacy-tasks.md and research.md are
historical migration inputs. Their checkboxes, transient paths, removed trace
switches and future-tense instructions do not authorize current work or prove
acceptance. No external document is copied into governing policy here.

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
