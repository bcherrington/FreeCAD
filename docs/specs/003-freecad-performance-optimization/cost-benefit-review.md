---
title: Owner-directed cost-benefit review of completed performance evidence
doc_type: spec
artifact_type: review
status: implementation-direction-selected
owner: local-developer
last_reviewed: 2026-11-08
---

# D007 — Existing Evidence, New Product Judgment

The owner selected [cost-benefit policy v2](acceptance-policy-v2.md) after seeing
these results. This is a retrospective direction decision, not a new predeclared
benchmark result. Original D001 outcomes, samples and validation files remain
unchanged. [Machine review](cost-benefit-review.json) binds the evidence and
records cell quantities separately from product dispositions.

| Change | Original D001 outcome | D007 direction | Remaining acceptance work |
| --- | --- | --- | --- |
| P04 direct conversion | Inconclusive from dispersion; numeric timing and resource thresholds pass | Retain; strong local conversion benefit supported despite noisy absolute timing | Final integrated/platform applicability; no rerun merely for the old 10% limit |
| P05 fallback conversion | Five large resource failures; all timing inconclusive; small resources pass | Retain; owner selects the speed/memory trade-off direction | Materialized-image count, temporary overlap, real workflow frequency and integrated memory pressure |
| P07 lazy full resolution | Original first-show/control failures and timing uncertainty remain | Retain for hidden-opening and RAM benefits | User-perceived first-use/tails and complete workflow applicability; n=2 diagnostic does not close acceptance |
| P01 buffered restore | Primary benefit; BIM p95 failure and other control uncertainty | Retain demonstrated payload benefit | Absolute control/tail cost and applicability; host-load cause remains unproved |

## P04: Useful Benefit Despite Variability

Primary medians improve 928→92 ms and 770→80 ms; median paired savings are
826 and 693 ms. Paired-bootstrap 95% after/before ratio intervals are approximately
0.090–0.107 and 0.089–0.108. These support a large local benefit. Absolute IQR/median
is 31.1%→15.5% and 17.9%→16.9%; control 162.1%→102.6%, with optimized absolute IQR
about 12–14 ms. The original dispersion flags remain correct. They do not erase a
large supported effect under D007. No universal 8–10x or idle-host claim follows.
All P04 resource gates passed. Retain with final applicability review in T010.

## P05: Accept The Direction, Account For Scaling

For the five 4096 cases, median paired savings are approximately 276–505 ms per
conversion. Every case adds 64 MiB peak logical decoded storage for the intermediate
RGBA image. Its logical allocation lifetime ends after conversion; allocator
caching can keep RSS elevated, and long-lived reuse/reclamation was not measured.
Grayscale Coin payload
also grows48 MiB (indexed gray) or32 MiB (gray-alpha/Gray16), retained while the field
is materialized. Premultiplied/SVG Coin payload stays unchanged. Measured process
peak RSS also increases; use the recorded before/after values, not a sum of logical
counters. All five large resource failures remain visible in the v1 record.

The owner prefers retaining this performance gain rather than automatically
rejecting it at 1 MiB. Workload frequency and many-image concurrency remain unknown;
this is not blanket memory approval. For illustration, ten independent materialized
4096 indexed-gray fields add480 MiB of logical Coin payload; ten gray-alpha fields
add320 MiB. One active conversion's additional decoded temporary is64 MiB. These are
payload accounting examples, not measured whole-application RSS or GPU estimates;
GUI copy overlap and other allocations also require review.

T004.4 must document the relevant materialized-image count, overlapping work,
operating-memory scope and rationale/mitigation using applicable evidence or
explicitly justified accounting. Reducing the temporary or retaining compact storage is an optional follow-up
when its benefit justifies added complexity; the old 1 MiB cap does not mandate
a rewrite. Preserve speed and fidelity if changing that implementation. Existing 512
controls pass resource gates. No production conversion algorithm is changed here.

## P07 And P01: Weigh The Complete Workflow

For P07 the corrected diagnostic's hidden opening4094→2071 ms and about 87 MiB lower
peak RSS favor retention; first reveal85→1012 ms is a real deferred cost. How often
images remain hidden versus are eventually revealed matters. First-use/tail review
remains mandatory, with the diagnostic's two observations per arm clearly limited.
Original T020 results remain separate. Faster opening alone is not a complete
responsiveness argument, and full resolution/calibration remain mandatory.

P01's primary payload and console-open benefits remain useful. Its BIM p95 rise
2785→3348 ms is about 563 ms of absolute tail cost to review, not evidence of host-load
causation. Neither the new policy nor unrelated host telemetry proves that cause.

## Ownership And Next Work

Correctness/fidelity/model identity, visible error behavior and qualified evidence
remain hard gates. T004 stays in progress. T004.3 parser/tree/save comparisons are
next, with the D007 review plan frozen before any samples. T004.4 then resolves
all retained changes' workflow trade-offs and operating scope; T010 checks final
integrated revisions/platforms. No production approval or new benchmark run is
claimed by this direction review.
