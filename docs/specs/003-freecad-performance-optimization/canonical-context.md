---
title: FreeCAD performance optimization requirements canonical context
doc_type: spec
artifact_type: canonical-context
status: draft
owner: local-developer
last_reviewed: 2026-11-08
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
| open-decisions.md | Authoritative decision status/ownership | D001–D005 | D001–D004 planning/selection resolved; D005: user retains P07; validation/first-use-tail criteria remain open; T022 precedes T004/T010 |
| validation-manifest.md and validation-manifest.json | Candidate identity and evidence applicability | T019 initial local Linux candidate | Historical binaries ineligible; actual new-build/runtime receipt required |
| benchmark-protocol.md; benchmark-plan.json; benchmark-runner.py | Predeclared measurement contract and bounded collector | T001/D001 | No FreeCAD probe, generated fixture, qualified arm or performance result claimed |
| tasks.md | Four-phase delivery queue and explicit next action | This package | T012/T013/T003/T006/T014/T017/T007/T015/T016/T005/T018/T002 complete; T020 measurement/disposition complete; T022/D005 is the mandatory next gate before T004; phase order/dependencies govern execution |
| performance-results.md; performance-results.json; t020-validation.json | Current aggregate two-campaign measurement/disposition and exact historical record links | T020; D001; D004; D005 | T020 evidence complete; recovery-2 valid but P07 rejected; no pooling, waiver or production acceptance |
| t020-recovery-2-plan.md; performance-recovery-2-results.md; performance-recovery-2-results.json | Corrected-probe plan and completed measured outcomes | T020; D004 | Seven valid cells/336 observations, 42,290-hash audits; timing failures/inconclusive outcomes, resource passes; no P07 acceptance |
| t012-validation.json | Local image extent/access qualification receipt | T012 plus actual working diff/source/binary hashes | No native GUI, calibration, full-format or performance acceptance implied |
| t013-validation.json | Local calibrated image-plane lifecycle receipt | T013 plus actual source/new-test/diff/binary hashes | No native viewport, broad image qualification or performance acceptance implied |
| t003-validation.json; t003-render-contact.png | Expanded byte/access/rendered fidelity receipt and visual artifact | T003 plus actual source/binary/context/output identities | Local native-X11 and offscreen software-GL framebuffer correctness; interactive frame/resource/performance and platform gates remain |
| t006-validation.json | Expanded deferred-load/error/calibration receipt | T006 plus actual source/diff/build/runtime identities | 24 Qt lifecycle passes, current bitmap/render regression checks; lifecycle Memcheck unavailable due Qt/IBus startup error |
| t014-validation.json | Receiver lifetime correction and qualification | T014 plus actual source/new-test/diff/build/runtime identities | Four functional rows pass normally and under Memcheck; no Windows test registration or broad scheduler acceptance |
| t017-validation.json | Restore scheduler and observer completion receipt | T017 plus exact source/new-test/diff/build/runtime identities | Ten functional rows pass normally and under Memcheck; larger-model/object semantics and final platforms remain T007/T010 |
| t007-validation.json | Current tree functional qualification and pinned input identity | T007/T007.1 plus actual new-test/registration/fixture/build identities | 25 cases pass natively, seven CTest targets and seven focused Memcheck cases; B011 remains deferred |
| t015-validation.json | Numeric restoration and boundary ownership correction | T015 plus actual tracked/private-header diff and final App build/runtime identities | Legacy strtol value parity; 277 naming/hash cases, 290 CTests, 30 Memcheck cases and exact-header ASan/UBSan; broader identity qualified by T016; T005 records aggregate acceptance |
| t016-validation.json | Writer/alias/child-map and dependent CAD reference qualification | T016 plus exact source/new-test/diff/build/runtime/fixture identities | 281 naming/hash tests, 294 CTests, 34 Memcheck cases; two CAD save/reopen cycles with three forced recomputes; T005 records aggregate acceptance; T010 confirms final applicability |
| t005-validation.json | Aggregate Requirement 4 / CP-005 acceptance and applicability audit | T005 at committed T016 root 95d1df147f / docs e3bf027 | 75 identity/integrity checks bind unchanged converter and final T016 source/binaries/dependencies/fixtures; reuses executed T015/T016 evidence; no new runtime pass |
| t018-validation.json | Included-file document failure policy, injection and metadata/save audit | T018 plus actual test/build/runtime and generated fixture identities | Preserved empty/nonempty reader disposition, partial bytes/permissions/notifications and later-entry restoration; T002 records aggregate acceptance |
| t002-validation.json | Aggregate included-file compatibility and direct byte/hook qualification | T002/T002.1/T002.2 at committed T018 base plus exact new test diff and final source/build/runtime/fixture identities | Four raw ZIP/reopened-file boundaries, six direct hook cases and all T018 document rows; 310 App/26 Reader/349 CTest/29 Memcheck; T020 later records rejected P07; T022/D005 now gates further comparisons |
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
has completed T012/T013/T003/T006 and the T014/T017/T007 tree subgroup. T015
has completed numeric safety and T016 has qualified writer/alias/child and dependent CAD identity; T005 completes aggregate local compatibility. T018 qualifies the preserved document-level archive failure policy; T002 and its children complete aggregate included-file compatibility. All Phase 2 correctness subgroups are locally complete; The incomplete first campaign is preserved; the corrected campaign completed all
required measurements and rejected P07 under D001. T020 measurement/disposition
is complete; T022/D005 now gates material choice and evidence planning before T004. D002's B009 routing and B010/B011 optional investigations do not
approve new behavior or waive any current MoE safety/measurement requirement.
The original manifest's "remaining decisions" prose records its intake time;
current status is always the active decision ledger.

## Shared Coordination Handoff

The user-supplied external handoff is
`../../ie/skunk-works/freecad-modelling-mcp/artifacts/freecad-crash-coordination/COORDINATION.txt`,
resolved from the FreeCAD repository root. It records chronological shared
investigation evidence, not canonical spec/source or fresh live-runtime authority.
Append dated, scoped findings and preserve previous evidence. Spec 003 progress
has been acknowledged there. Its separate D600 native geometry-read profiling
request remains unexecuted by T015; parser/tree qualification does not establish
a native profiling result, MCP crash fix or installed operator-runtime update.

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
