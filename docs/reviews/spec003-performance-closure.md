---
title: Spec003 local performance delivery and closure review
doc_type: review
artifact_type: review
status: active
owner: local-developer
last_reviewed: 2026-11-09
---

# Spec003 Local Performance Delivery And Closure Review

## Disposition

T011 promotes locally accepted document/image behavior and D007 cost-benefit
rules into the [reference](../reference/document-open-performance.md) and
[runbook](../runbooks/freecad-performance-optimization.md). Independent expert
review cleared substantive findings; final guarded state, hash and commit checks
record the retained-in-place local closure disposition. This review
never grants upstream, release or cross-platform approval.

## Qualified Implementation And Evidence

The qualified integrated source is commit `21903ca31f`; matching committed docs
are `756c03a`. Its five corrective files exactly match the T010 v3 base-plus-patch
source hashes. No source, dependency, fixture or runtime change is made by T011.
The shared build inventory now differs for libFreeCADGui and PartGui (16 of 18
selected binaries still match). Their new hashes are observed, not qualified.
Original test results apply to the recorded T010 binaries; this documentation-only
closure does not certify later shared-build outputs. A new runtime claim requires
affected requalification.
The [T010 report](../specs/003-freecad-performance-optimization/t010-results.md)
and immutable receipt retain the real build, focused/GUI/CAD/memory and full
qualification: 2436 registered tests, 2426 passed, zero failed, two existing skips
and eight disabled. Disabled/skipped tests are not passing coverage. No new build,
GUI run or timing campaign is needed for documentation-only promotion.

The accepted native viewer/memory profile is Linux x86_64, X11 softwareGL,
compose input and Qt generic/Fusion, with user-site Python masked and actual
mapped modules bound to the final build. Windows/macOS/Wayland/other drivers and
GNOME/IBus memory remain unqualified. Scoped Memcheck proves zero access errors
for 26 Qt cases with the existing libunwind probe suppression; leak and
undefined-value analysis are excluded.

Original D001 protocols, results and classifications stay unchanged. D007's
retrospective local decision accepts directions, hidden/subset first-use and
logical-memory tradeoffs, with the P05 three-plane/one-conversion headroom bound.
It does not turn original failures into statistical passes. P01/P07 historical
external embedded-Python DSO closure is incomplete, and their numeric timing,
tail and RSS figures are historical observations. The old load classifier
omitted App-only restore. Corrected-source red/green tests and fresh stage traces
supply current lifecycle proof. P02/P03 isolated workers and full maps exclude
that user-site concern; no exact-final whole-workflow timing claim is introduced.

## Acceptance And Promotion

[Traceability](../specs/003-freecad-performance-optimization/traceability.md)
reconciles all six requirements/26 acceptance criteria, CP-001–CP-005,
SC-001–SC-005 and F001–F009 to completed local qualification or explicitly
reviewed applicability/exclusion. The closure receipt enumerates every criterion
and its evidence. The promoted reference contains the accepted archive/error,
image/conversion/calibration, map identity and tree/lifetime contracts, not just
links to a temporary spec. The runbook owns future validation, recovery,
process/runtime isolation and storage reuse rules. The storage runbook remains
current; archival does not authorize deleting builds or evidence.

## Owned Residuals

| Record | Owner and disposition | Boundary |
| --- | --- | --- |
| B009 | local-developer; deferred reduced-resolution proposal | No resolution reduction implemented or accepted |
| T008 → B010 | local-developer; deferred conditional GUI/startup investigation | No new investigation executed; original activation/exit criteria retained |
| T007.2 → B011 | local-developer; deferred further tree/bulk-delete profiling | No larger-model further-optimization profile or bulk-delete bypass approved |
| B012/B013 | completed locally by T010 source commit 21903ca31f | Actual Measure initialization and finite-edge loft repair; old failed runs preserved |
| Future scope extension | local-developer under promoted reference/runbook | Other platforms, stricter timing claims or wider P05 inventory/concurrency need affected qualification before approval |
| External delivery | local-developer; separately scoped future delivery | Local expert review is not actual maintainer acceptance; no push, PR, install or live operator delivery performed |

All optional records have durable destinations in the [backlog](../backlog/README.md).
Current hard-gate failures may never be hidden in optional backlog routing.

## Closure Tool Disposition

Spec Lifecycle Manager 0.6.1's installed `lifecycle/core.py` implementation of
`closure_check` calls `task_verified` for every task; that predicate accepts only
complete-with-evidence records. Thus valid `[>]` routing is reported as
`TASK_NOT_VERIFIED`, even though its task audit allows owned follow-ups as final
child dispositions. This is a tool limitation, not a passed closure check.

T011 explicitly dispositions only the T008 and T007.2 findings against the
repository authoring contract, which allows routed/deferred outcomes before
closure. The records remain `[>]`, with destinations, accountable owner,
activation and exit checks verified. They are neither executed investigations
nor completed implementations. Any other tool blocker must be resolved rather
than covered by this narrow disposition. The final receipt records the exact
tool result and implementation hash; installed plugin code is not changed.

## Historical Authority And Package Disposition

Repository convention retains closed packages in place. Current guidance lives
in durable docs; old checkpoint/next-task prose, policies and receipts remain
historical. T010's receipt intentionally binds its completion-time mutable
requirements/tasks/traceability/reference bytes. The T011 receipt resolves those
bindings through actual commit `21903ca31f`; it does not rewrite old hashes to
match newly promoted docs. Earlier T004.4 snapshots remain retained as before.

The closure log and archive index record the actual implementation/evidence
commits and explicitly pending promotion/cleanup commit identifiers. No future
commit identity is invented. Final commit reconciliation is required when the
user requests a commit; pending Git metadata does not imply pending code/testing.

## Independent Review And Validation

Operations/documentation/process and performance/provenance/QA experts inspect
actual final artifacts. Review findings and final direct validation are recorded
in the T011 receipt. Agent Workbench remains navigation-only at the nested-docs
boundary; blocked/skipped navigation checks are not claimed as validation.
