---
title: Cost-benefit performance acceptance policy
doc_type: spec
artifact_type: acceptance-policy
status: owner-selected
owner: local-developer
last_reviewed: 2026-11-08
---

# Cost-Benefit Acceptance — D007

Effective 2026-11-08, the owner requested relaxed criteria with greater emphasis
on cost-benefit after reviewing P04 variability and P05 memory costs. This policy
is the current product-decision layer for Spec 003. It supersedes D001's blanket
numeric vetoes for current/future acceptance decisions, including dispersion,
relative latency, first/repeat-show and resource limits. It does not alter any
measurement, original rule, assessor or historical PASS/FAIL/INCONCLUSIVE result.

This is an explicitly **posthoc owner policy decision** for existing evidence.
Its retrospective review can select a useful implementation direction; it cannot
turn old samples into a predeclared v2 statistical pass. It is not production or
maintainer sign-off. T004 and T010 remain open for the declared residuals.

## Three Separate Judgments

1. **Correctness and trustworthy evidence — mandatory.** Preserve pixel/alpha,
   orientation, calibration, model/reference identity, archive/error behavior,
   lifetimes and safe arithmetic. Require qualified source/build/runtime identity,
   applicable workflow coverage, bounded collection and complete failure records.
   Speed cannot compensate for a failed hard gate. Historical inapplicability
   dispositions remain explicit; missing required coverage is not a pass.
2. **Measured benefit and uncertainty.** Report absolute and relative changes,
   paired confidence intervals, median and tails, dispersion and observed host
   conditions. Distinguish decode/conversion/open/first-show/repeat. A clear paired
   benefit with substantial absolute savings is useful despite noisy absolute
   timings. Uncertain evidence supports only correspondingly limited claims.
3. **Workflow cost-benefit and operating scope.** Weigh user-visible saved time,
   operation frequency, open-plus-first-use cost, images never revealed, peak versus
   retained memory, simultaneous images/conversions, implementation complexity and
   maintenance risk. Select an explicit disposition for a stated scope.

## Review Triggers Replace Automatic Numeric Rejection

The original IQR/median >10%, speedup floors, median/p95 non-regression limits,
500/1000 ms first-show and 50/100 ms repeat-show targets, RSS allowance and 1 MiB
Coin/decoded allowances remain reported comparison points. Breaching them triggers
a documented review, rather than automatically vetoing an otherwise useful change.
Changing the percentage to a larger arbitrary percentage is not the new policy.
There is no invented universal exchange rate between milliseconds and megabytes.

Every triggered review must record:

- The original outcome/threshold breach, fixture, source/profile and complete
  evidence reference, with absolute quantities as well as percentages.
- Benefit, frequency and tail behavior; a median gain cannot hide an operationally
  harmful stall or a control regression. Record unknown frequency explicitly.
- Temporary decoded allocations, retained Coin payload and measured process RSS
  separately; GPU residency is unmeasured. Review materialized-image count,
  overlapping conversions and memory pressure. Do not add these metrics together
  as if they were independent allocations or infer whole-application RSS by sum.
- Confidence, variability, sample-count and applicability limits. Host load is
  context, not proof that an inconvenient observation was caused by contention.
- Why the benefit warrants the observed cost within the selected scope, mitigation
  or monitoring, unresolved evidence, and the accountable owner/follow-up task.
- A decision: retain for a stated scope, revise, defer pending evidence, or reject.
  Production acceptance requires the applicable hard gates and resolved operating
  scope. An accepted direction is not blanket workload or platform approval.

P05 temporary QImage allocation lifetime ends after conversion, while expanded
grayscale Coin payload remains while materialized. Allocator caches/driver storage
can keep process RSS elevated; these fresh-process peak samples do not establish
long-lived reclamation or reuse behavior. Many-image scaling therefore deserves a
separate decision from accepting a single transient allocation. P07 can trade a
slower first reveal for faster hidden opening and lower memory when images remain
unshown; user-perceived first-use/tail behavior still requires explicit review.
No resolution reduction or calibration change is authorized by this policy.

## Evidence Reuse And Future Sampling

Keep benchmark-plan.json, benchmark-protocol.md, benchmark-assess.py, all completed
campaign files and their result/validation bindings unchanged. Their labels are
**D001 v1 outcomes**, not the current policy's product dispositions. Store new
cost-benefit judgments separately in cost-benefit-review.md/json. Preserve the
pre-D007 mutable authority snapshots bound by completed T004.2 validation.

Existing complete qualified evidence may inform the explicitly retrospective
review. Do not repeat a campaign merely to clear the old 10% or 1 MiB veto.
Source, build, fixture, workflow or environment changes still require affected
requalification/new measurements or an exact reviewed equivalence. Incomplete
failed runs and the n=2 P07 diagnostic do not gain acceptance-sample eligibility.

Before T004.3 or other future samples, freeze this policy's identity/hash together
with the comparison plan: primary/control inventory, endpoints, sample counts,
confidence/statistics, review triggers, absolute timing/resource metrics and all
trigger-resolution fields above. Keep the established collection safeguards and
sampling defaults unless a separately recorded pre-sample change selects otherwise.
No retries, posthoc exclusions, pooling or adaptive sampling to search for a pass.

The unchanged v1 assessor may provide legacy diagnostic classifications. Its
output alone cannot certify D007 acceptance. Future execution must bind the
policy/review plan and retain separate statistical outcomes and owner-reviewed
product dispositions; wire that verification before collecting new samples.

## Current Direction And Follow-Up

[cost-benefit-review.md](cost-benefit-review.md) records the existing evidence's
secondary review. Retain P04, P05 and P07; retain P01's demonstrated payload benefit
while its control/tail impact remains under review. P04 needs no new campaign
solely to satisfy the old variability limit. P05's speed/memory direction is chosen,
with realistic memory/concurrency applicability still required. P07 responsiveness
and integrated-source evidence remain required. T004.3 remains next; T004.4 owns
consolidated workflow trade-off/operating-scope review, and T010 final applicability.
