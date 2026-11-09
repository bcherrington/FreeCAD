---
title: Mainstream CAD investigation canonical context
doc_type: spec
artifact_type: canonical-context
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# Canonical Context

## Purpose

Keep mainstream user latency, historical motivation, investigation results and
future implementation authorization distinct. This package is newly authored;
no current bottleneck or speedup is established by its existence.

## Authority Hierarchy

User instructions and root/deeper AGENTS.md govern work. Repository templates
under docs/templates/spec-package govern artifact shape. Requirements define
investigation acceptance; design/tasks/verification must remain consistent.
Current source and tests establish observed behavior, but a mismatch with an
accepted contract requires reconciliation and cannot silently redefine it.

## Current Authoring Baseline

- Root baseline: `9c4b3d049e`; docs baseline: `30c9290` at intake.
- Preserved unrelated edit: docs/design/python-console-standalone-window.md.
- Existing untracked Python caches belong to other work; do not stage or remove them incidentally.
- Spec000/003/004 are retained history; their frozen receipts are not rewritten.
- Shared binaries may change independently of source; actual hashes/module closure are T001/T002 gates.
- No application build, benchmark, GUI mutation or production code change is part of spec creation.

## Spec-Canonical Working Sources

| Source | Authority and scope | Use boundary |
| --- | --- | --- |
| requirements.md | Current investigation requirements and correctness properties | No runtime completion inferred |
| design.md | Proposed workload/endpoint/measurement/decision contracts | Exact fixtures/probes frozen by T001/T002 |
| tasks.md | Linear T001–T012 queue | T001 complete; T002 next; do not hide partial work |
| verification.md; traceability.md | Coverage and readiness gates | Lint/context are not measured evidence |
| open-decisions.md | D001 resolved; D002 owned by T002 | Qualification decisions resolved prospectively |


## Ownership And Boundaries

local-developer owns this investigation and all candidate dispositions. B001 maps
to T005; B011 maps to T009. Existing backlog entries link here while preserving
truthful not-yet-executed status; their detailed acceptance is not discarded.
No image workload, startup diagnosis, new QR/default/threshold policy, bulk-delete
bypass, global parallelism, cache framework or public API is selected.

Agent Workbench is navigation/currency input, especially at the nested-docs
boundary. Direct source/manifest checks and executed receipts establish evidence.
Use actual declared tools/capabilities, not assumed profiler/runtime APIs.

## Current Next Action

T001 is complete and D001 resolved. Current frozen inputs are
[workload-manifest.json](workload-manifest.json),
[measurement-plan.json](measurement-plan.json) and the durable public canonical
files. [T001 results](t001-results.md) and its validation receipt document actual
preparation, provenance and limits. The prior authoring receipt is an immutable
checkpoint whose mutable document bindings resolve through root `877ebdfb3e`.

T002 must qualify actual Release binaries, loaded runtime, runner/endpoints,
full semantic oracles and resource feasibility, then resolve D002 before samples.
Keep the v1 input bytes unchanged; new inputs/policies need a prospective version
and recorded review, not retroactive replacement. No timing result is claimed.

## Always-Canonical External Sources

| Source | Authority reason | Handling |
| --- | --- | --- |
| Root AGENTS.md and applicable deeper instructions | Repository development rules | Preserve unrelated work and follow normal build/line-ending rules |
| docs/templates/spec-package/README.md | Repository spec format | Governs artifact roles and stable task/evidence semantics |
| docs/reference/document-open-performance.md and existing runbooks | Accepted compatibility, cost-benefit and collection/storage rules | No waiver from an exploratory result |
| Current source/tests and exact execution records | Implemented behavior and actual runtime | Reconcile mismatches with accepted contracts; do not silently redefine them |
| ../../reference/document-open-performance.md | Promoted hard contracts and cost-benefit policy | Historical P01/P07 numbers are not current timing guarantees |
| ../../reference/sketcher-performance-diagnostics.md | Accepted diagnostic behavior and prior qualification | Current interfaces/settings still need inspection |
| ../../backlog/sketcher-performance.md#b001-qr-method-comparison | B001 original scope and exit checks | Historical QR shares motivate comparison, not sparse superiority |
| ../../backlog/spec003-follow-ups.md#b011-further-tree-batching-or-bulk-delete-profiling | B011 original scope and invariants | Profiling is selected here; bulk deletion remains unapproved |

## Imported Sources

| Spec path | Source path | Source revision or date | Status | Canonical scope | Promotion target |
| --- | --- | --- | --- | --- | --- |
| requirements.md; design.md | docs/backlog/sketcher-performance.md B001 | 2026-11-09 authoring | adapted | Existing QR comparison invariants and exit checks | Existing Sketcher reference/backlog plus new mainstream reference |
| requirements.md; design.md | docs/backlog/spec003-follow-ups.md B011 | 2026-11-09 authoring | adapted | Larger-model tree profiling and safety boundaries | B011 plus new mainstream reference/runbook |
| design.md | docs/reference/document-open-performance.md | Root 9c4b3d049e intake | summarized | Cost-benefit/hard-gate and runtime provenance rules | New mainstream reference/runbook with original contract links |

## Non-Canonical Background Sources

| Source | Reason non-canonical | Handling |
| --- | --- | --- |
| Spec000 archived QR/shape attribution | Different workload/source and earlier measurement scope | Motivation only; do not reuse as new stream baseline |
| Spec003 archived campaigns and T010 runtime hashes | Historical source/profile and known later shared-output drift | Retain unchanged; qualify actual current binaries/fixtures |
| Earlier chat estimates or speculative optimization ideas | Hypotheses, not measured evidence | Test through selected stream or reject with rationale |

## Promotion Map

| Spec-local content | Durable destination or route | Required before closure |
| --- | --- | --- |
| Accepted workload/stage findings and boundaries | docs/reference/mainstream-cad-performance.md | yes |
| Measurement/reset/oracle/isolation procedure | docs/runbooks/mainstream-cad-performance-investigation.md | yes |
| B001/B011 final evidence and decisions | Existing backlog entries; one owned implementation destination if pursued | yes |
| Raw audit evidence and closure identity | Retained package and history log/index under repository policy | yes |

## Completion-Time Evidence

The T001 receipt binds completion-time mutable documentation and immutable v1
inputs. Later task routing/documentation changes resolve older bytes through the
actual T001 package commit in Git history, rather than rewriting old receipt
hashes. Canonical fixture bytes are versioned with the package; ignored build
outputs are not the only recovery source.
