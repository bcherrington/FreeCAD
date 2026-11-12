---
title: Feature verification title
doc_type: spec
artifact_type: verification
status: draft
owner: team-or-person
last_reviewed: YYYY-MM-DD
---

# Verification

## Scope

Describe what implementation slice, requirements, and task IDs this verification
record covers.

## Quality Gates

| Gate | Required? | Status | Evidence |
|------|-----------|--------|----------|
| Requirements acceptance criteria reviewed | yes | pending | |
| Task evidence complete | yes | pending | |
| Automated tests pass or alternate verification recorded | yes | pending | |
| Durable documentation updates identified | yes | pending | |
| Durable documentation promoted or explicitly deferred | yes | pending | |
| Spec cleanup decision recorded | yes | pending | |
| Governance or policy conflicts resolved | yes | pending | |

## Validation Commands

| Command | Purpose | Result | Evidence |
|---------|---------|--------|----------|
| `command` | What it validates | pending | |

## Requirement Coverage

| Requirement | Acceptance criteria covered | Evidence | Residual risk |
|-------------|-----------------------------|----------|---------------|
| Requirement 1 | AC1 | | |

## Correctness Property Coverage

| Property | Covered by | Evidence | Residual risk |
|----------|------------|----------|---------------|
| CP-001 | Task, test, command, review, or manual method | | |

## Scope Reconciliation Before Closure

Use this section before promotion or closure for Must requirements,
architectural targets, migrations, public interfaces, cross-module behavior, or
review findings whose wording is broader than one implementation task. Compare
the requirement/design target against actual task evidence, not only checkbox
state.

Coverage states:

- `complete`: implemented and verified in this spec.
- `partial-routed`: accepted partial coverage with remaining work routed to one
  explicit destination that does not block closure.
- `partial-blocking`: partial coverage that blocks closure until completed,
  rejected with rationale, or routed.
- `not-covered`: no evidence of implementation; blocks closure unless rejected
  with rationale or routed to one destination.
- `out-of-scope`: intentionally outside this spec, with rationale and one
  durable destination when follow-up work remains.

| Broad requirement, design target, or review finding | Implemented in this spec | Coverage state | Deferred or rejected work | Destination | Blocks closure? | Evidence |
|-----------------------------------------------------|--------------------------|----------------|---------------------------|-------------|-----------------|----------|
| Requirement/design/review item | Task IDs, code, docs, tests, or none | complete \| partial-routed \| partial-blocking \| not-covered \| out-of-scope | Remaining work or rejection rationale | backlog, roadmap, issue, follow-up spec, owner, or none | yes/no | command, review, task evidence, or artifact |

## Agent Readiness Evidence

| Field | Evidence | Residual risk |
|-------|----------|---------------|
| Scope and out-of-scope files | | |
| Must-read and optional context | | |
| Permissions and approval points | | |
| Validation commands and expected signals | | |
| Review needs | | |
| Durable-doc or closure impact | | |
| Optional repo-evidence provider caveats | | |

## Task Evidence

| Task ID | Status | Evidence | Notes |
|---------|--------|----------|-------|
| T001 | pending | | |

## Evidence Log

| Date | Evidence | Result | Notes |
|------|----------|--------|-------|
| YYYY-MM-DD | Command, review, manual verification, or artifact | pending | |

## Manual Or External Verification

Record any verification that is not an automated local command, including who or
what performed it, the date, evidence, and residual risk.

## Residual Risks

- Risk and mitigation or owner.

## Durable Promotion And Cleanup

Use this section before closure. A spec is not ready to close while accepted
behavior, decisions, operations, contracts, validation procedures, or follow-up
ownership exist only inside the spec package.

Before marking closure ready, confirm every `partial-routed`,
`partial-blocking`, `not-covered`, or `out-of-scope` row above has exactly one
destination or a rejected-with-rationale disposition. `partial-blocking` and
`not-covered` rows block closure unless their disposition is updated.

| Spec content | Durable destination or deferral | Status | Evidence |
|--------------|---------------------------------|--------|----------|
| Requirements and accepted behavior | `docs/requirements/...` or target repo equivalent | pending | |
| Technical design or architecture | `docs/design/...` or `docs/architecture/...` | pending | |
| Contracts, schemas, data flow, or integration behavior | contract/data-flow/reference docs | pending | |
| Operational steps, rollout, validation, or recovery | runbook/getting-started/checklist docs | pending | |
| Decisions and rationale | ADR, history note, or durable reference | pending | |
| Follow-up work | backlog, roadmap, issue, or follow-up spec | pending | |

### Spec Cleanup Decision

- **Cleanup action:** keep active | archive | remove | retain as history note
- **Reason:**
- **Final spec commit:** pending | commit hash | not required
- **Closure log path:** pending | `docs/history/spec-closure-log.md` | repository-specific path | not required
- **Closure log entry updated:** no
- **Closure cleanup commit:** pending | commit hash | not available yet
- **Active indexes updated:** no
- **Durable docs linked back to evidence where useful:** no
- **Residual spec-only content:** none | listed below

Residual spec-only content:

- Content and owner or reason for retention.

## Ship Or Closure Risk

- **Risk level:** low | medium | high
- **Breaking change:** no
- **Blast radius checked:** no
- **Rollback path:** not required | documented | unavailable
- **Requires human review:** no
- **Release notes needed:** no
- **Follow-up issue or spec needed:** no

### Risk Rationale

Explain why this risk level is appropriate and what evidence supports it.

## Readiness Decision

- **Ready for promotion:** no
- **Ready for release:** no
- **Ready for closure:** no

## Related Artifacts

- Requirements:
- Change Impact:
- Design:
- Tasks:
