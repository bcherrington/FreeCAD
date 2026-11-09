---
title: Review FreeCAD document performance tradeoffs
doc_type: runbook
status: active
owner: local-developer
last_reviewed: 2026-11-09
---

# Review FreeCAD Document Performance Tradeoffs

## Purpose

Maintain scoped cost-benefit decisions without turning numeric review triggers
into blanket vetoes or discarding fidelity and provenance requirements.

## When To Use

Use when changing retained document/image optimizations, reviewing performance
reports, extending image-count/concurrency scope, or confirming final applicability.
This procedure grants no new production approval by itself.

## Prerequisites

- Access: local source, qualified receipts and prior immutable results.
- Environment: the declared platform/build/runtime profile; record differences.
- Tools: Spec Lifecycle Manager for task state/traceability, Agent Workbench for
  scoped navigation, Git and hash/accounting checks. Native tests are needed only
  for applicable source/runtime gaps, using repository pixi workflows.

## Inputs

Change identity, source/build/runtime and fixture hashes, declared workflow,
image formats/dimensions/materialized count, overlap, available memory headroom,
all D007 review fields and an accountable owner. Keep private fixture paths and
contents out of public artifacts.

## Linked Docs

| Document | Why It Matters |
| --- | --- |
| [Scoped reference](../reference/document-open-performance.md) | Current decisions, limits and memory formulas |
| [T004.4 review](../specs/003-freecad-performance-optimization/t0044-cost-benefit-review.md) | Per-change rationale and complete trigger closure |
| [D007 policy](../specs/003-freecad-performance-optimization/acceptance-policy-v2.json) | Mandatory hard gates and eleven required review fields |
| [Storage runbook](freecad-benchmark-storage.md) | Reuse one qualified pair and preserve aborted evidence |

## Config And Code Touchpoints

| Area | Path Or Setting | How It Is Used | Required Validation |
| --- | --- | --- | --- |
| Included files | `src/App/PropertyFile.cpp` | Buffered restore/save path | Bytes/ZIP/errors/permissions and scoped workflow evidence |
| Element maps | `src/App/ElementMapRestore.h`, `src/App/ElementMap.cpp` | Safe parser and identity behavior | Qualified grammar, overflow and geometry-manifest identity |
| Image conversion/lifecycle | `src/Gui/BitmapFactory.cpp`, `src/Gui/ViewProviderImagePlane.cpp` | Materialization and decoded/Coin lifetimes | Pixels/orientation/calibration/ownership/pending-load checks |
| Tree lifecycle | `src/Gui/Tree.cpp` | Restore/import fallback and finish behavior | Receiver lifetime, safe fallback and finish/flush checks |
| Evidence contract | Spec003 policy, campaign receipts and task queue | Read pinned hashes; advance current authority only | Immutable history, reviewed applicability and SLM lint/traceability |

## Procedure

1. Read the current reference and exact source-bound receipts. Distinguish local
   conversion/hooks, console open/save, GUI first frame and whole workflow.
   Identify eligible source/fixture/runtime evidence and any actual applicability gap.
2. Preserve original assessments, policy bytes and aborted/ineligible attempts.
   Archive exact completion-time authority before advancing mutable docs; never
   pool retries, silently reclassify failures or infer host causality from aggregate load.
3. For each P01–P08, record all D007 fields: trigger, source scope, absolute/relative
   benefit, confidence/tails, frequency, transient/retained/process-peak memory,
   overlap/scope, uncertainty, rationale/mitigation, owner/residuals and decision scope.
   Adopt source-reviewed P06/P08 exclusions without inventing independent speedups.
4. For fallback images, inventory materialized counts, formats, dimensions, active
   conversions and replacements. Apply the reference's per-plane accounting;
   include the unmeasured/base allocations in actual headroom judgment. Outside
   the selected scope, record an extension decision and evidence needed. No numeric
   memory allowance or review limit is a new software clamp or universal RSS ceiling.
5. For lazy images, distinguish images never shown from deferred first use, repeat
   display and immediate-all-visible workflows. Disclose the full measured first-use
   distribution. Do not replace p95/maximum evidence with an exploratory n=2 sample
   or a new one-second promise. Retain full resolution and calibration.
6. Have relevant performance/QA, CAD/lifecycle and operations/documentation reviewers
   check decisions. Use accounting/equivalence where sufficient; collect new samples
   only for real gaps under a new predeclared bounded plan. Normal build/test work
   follows `pixi run build -j 2` and focused affected suites, then required broader gates.
7. Update current reference, review, task evidence and traceability. T010 confirms
   exact final composition/platform and invalidates only affected evidence; T011
   owns final promotion/closure. Do not mark the whole spec accepted from this review.

### Extending The Image Inventory

Record an extension packet with desired formats, exact dimensions, materialized
count, simultaneous conversions and old/new replacement overlap. For each image,
calculate extra retained bytes as `3 * width * height` for indexed gray or
`2 * width * height` for gray-alpha/Gray16. For each active fallback conversion,
account for `4 * width * height` temporary bytes. Sum the relevant inventories
separately; zero retained delta for the tested premultiplied/SVG cases does not
mean zero total storage. These payload formulas generalize accounting, not
performance/fidelity qualification to untested dimensions or formats.

Record observed process baseline/peak and system available-memory context for
the intended workload if available. Explicitly list unavailable observations and
allocator retention, base model/decoded/Coin allocations, old/new copies and GPU
unknowns. The owner records date, available margin and a reasoned judgment of
whether accounting is sufficient for the desired scope. If an unknown can change
that judgment, collect only the relevant targeted evidence under a bounded
predeclared plan; do not approve the extension while that gap remains. Publish
a dated review/reference update with the selected scope and evidence. There is
no universal RAM threshold or mandatory full campaign rerun.

## Validation

Check every D007 field for every retained change/exclusion; reconcile the measured
values with bound source artifacts, verify logical memory arithmetic and explicit
unknown frequency/overlap. Verify original result and receipt hashes, links,
privacy and task-state consistency, then run package lint/traceability. Record
independent review and separate actual checks from navigation suggestions.

## Rollback Or Recovery

If applicability or fidelity fails, stop the affected acceptance claim and preserve
the evidence. Restore the prior documented decision or qualify an affected fix.
Retain the previous signed commits and archived authority; do not erase a failed
campaign or raw-revert shared lifecycle/tree safety repairs to manufacture a
counterfactual. Reuse the existing benchmark pair when new evidence is justified.

## Escalation

The local-developer owner resolves changed operating scope, insufficient headroom,
new strict latency requirements or incompatible provenance. A safety/fidelity/error
failure blocks affected acceptance regardless of speed. Exact final source or
platform uncertainty remains a T010 gate; closure remains T011.

## Final Native Runtime And Restore Checks

Use [the final source/runtime receipt](../specs/003-freecad-performance-optimization/t010-results.md)
for the accepted Linux profile and exact patch. Normal build probe options stay
OFF. Test App-only restore before GUI restore, GUI restore, import, first show,
repeat show and final document-owned viewer close. Ignore retained non-document
Start tabs when checking ownership; do not ignore missing-image or scene-removal
warnings. Trace and fix real failures before claiming clean qualification.

For isolated viewer checks, use a task-owned display and explicit process/window
identity; omit `--single-instance`, which can forward work to an existing session.
Publish generated public scenes only. Keep private model paths/content/frames out
of committed artifacts. Preserve failed harness, preflight and qualification
attempts separately, with their eligibility status.

FreeCAD's embedded Python explicitly enables user-site modules, so ordinary
environment flags alone do not establish isolation. The recorded validation uses
a process-only mount namespace to hide the user-site directory and verifies
actual mapped modules against the final build and selected lock/host closure.
Do not uninstall or modify user packages. Confirm actual dependency files rather
than equating installed package metadata with runtime closure.

Nested Pixi probing may fail inside that namespace. Activate and verify the locked
Pixi environment first, then use the exact CTest body from the repository task
under the mask, recording the diagnostic equivalence and all test outcomes.
Do not silently solve the lock, fabricate platform metadata or filter failing
tests. Existing source-level skips/disabled cases must remain explicit coverage
limits. The qualified native viewer/memory profile is compose + generic/Fusion
on X11 softwareGL; it is not an IBus/GNOME memory-safety certificate.

The old P01/P07 numeric results have incomplete external embedded-Python closure,
and old premature-load classification omitted App-only restore. Preserve them as
historical data. Use source-supported local directions and new functional gates
for scoped D007 decisions; freeze a new masked-runtime campaign before making
current absolute/tail/budget claims.
