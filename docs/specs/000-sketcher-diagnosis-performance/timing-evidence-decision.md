---
title: Indicative timing evidence decision
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Timing Evidence Decision - 2026-11-05

The user explicitly requested that timing evidence be toned down: the objective
is an indication of improvement, rather than strict performance measurement.
This changes AC-005/AC-006 and T005 acceptance. It does not change solver behaviour,
correctness requirements, actual execution checks, input authority or privacy rules.

Use comparable optimized builds, the same frozen operations/settings and disposable
model copies, warmups and several repeated samples. Record medians and spread,
execution counts and caveats. Look for a useful directional improvement supported
by the removed setup and timings; no fixed percentage or confidence-bound gate is
required. Review other workloads and tracing-disabled measurements for apparent
repeatable material slowdowns, investigating/dispositioning them if found. Noisy
or near-flat controls can be reported as uncertain without blocking completion.
Do not claim proven absence of regression or guaranteed overhead bounds.

Existing data already exceeds this evidence requirement: setups drop two to one,
master/profile median recompute is approximately 47%/57% lower, and correctness
checks pass. Other workload and overhead estimates are variable; no consistent
material slowdown attributable to the change is established. No further timing
runs or controlled-host cohort are required to finish T005 under this decision.

All original samples, per-batch/pooled statistics and former strict-gate outcomes
remain unchanged and labelled historical. The former 10%/5%/2% confidence gates
were not retrospectively passed; they are superseded as acceptance requirements
by this explicit user decision. The original collection protocol remains a record
of how those samples were obtained. T005 can complete; T006 is next. T007 still
requires attribution/review/promotion and closure work, rather than closing now.
