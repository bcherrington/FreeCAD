---
title: Mainstream CAD performance investigation
doc_type: spec
artifact_type: brief
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# Feature Brief

## Summary

Investigate the operations that dominate everyday CAD work, then choose the next
implementation from evidence. Scope covers parametric recompute; sketch editing;
patterns/booleans/fillets/chamfers; assemblies; viewport/selection; native model
open/save and distinct STEP import. It also explicitly includes B001 existing QR
comparison and B011 large-model tree profiling. Image optimizations are outside
this package. No speedup or production behavior change is promised.

## Read Order And Authority

1. [Canonical context](canonical-context.md): current authority, ownership and limits.
2. [Requirements](requirements.md): user outcomes, numbered acceptance and scope.
3. [Design](design.md): workload registry, shared harness and eight stream contracts.
4. [Tasks](tasks.md): stable linear queue and evidence requirements.
5. [Traceability](traceability.md) and [verification](verification.md): coverage and gates.
6. [Decisions](open-decisions.md): runtime planning blockers and owners.

Repository templates are authoritative. This brief is navigation, not a second
requirements source. No design-first exception or legacy migration is needed.

## Status

Draft investigation package. All T001–T012 are pending. Next: **T001 — Freeze
representative workloads, semantic oracles and the collection plan.** T001
resolves D001; T002 qualifies actual endpoints/runtime/harness and resolves D002
before any timed stream observation counts. Creation does not start profiling.
B001/B011 retain their existing IDs and correctness contracts; archived source
packages and other optional proposals stay unchanged.
