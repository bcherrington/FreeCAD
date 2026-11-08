---
title: Current performance measurement and acceptance status
doc_type: spec
artifact_type: verification
status: measured
owner: local-developer
last_reviewed: 2026-11-08
---

# Current Performance Measurement And Acceptance Status

T020 and T004.1 measurement/disposition are complete. **P01 and retained P07 are
not accepted under D001.** Parent T004 remains in progress and owns P01's
acceptance/evidence-quality residual. **Active: T004.2; next P04/P05 image conversion
comparisons after the completed exploratory P07 diagnostic**, then T004.3 parser/tree/save comparisons.
Phase 3, SC-005 and T010 production acceptance remain unsatisfied.

| Comparison | Valid measured evidence | Outcome / remaining owner |
| --- | --- | --- |
| P01 restore copy | All five cells, 240 fresh observations, twenty measured pairs per cell; both audits pass | Primary and EngineBlock pass; BIMExample fails open p95; ArchDetail and PartDesignExample inconclusive. P01 NOT ACCEPTED; T004/local-developer |
| P07 lazy full resolution | Two separately preserved campaigns; corrected campaign completes seven cells/336 observations | First-show and two control gates fail; other timing inconclusive; resource gates pass. Retain candidate under owner direction; T004.2/local-developer |

The [P01 report](t004-p01-results.md) records median payload restore 513.92 to
26.79 ms and complete console open 1183.27 to 704.44 ms on the primary. Its
BIMExample open p95 rises 2784.98 to 3347.69 ms, failing the non-regression rule.
These console measurements cannot stand in for GUI first-frame/first-show data.
D006 marks only the two structural-zero controls' payload metric not_applicable;
all five cells and their other gates remain required.

The [corrected P07 report](performance-recovery-2-results.md) retains first-show
median/p95 857.91/1843.09 ms against 500/1000 ms. Observed RAM savings and lower
opening medians support retaining the candidate; they do not waive failed timing
rules or establish an accepted overall speedup. Full resolution/calibration and
zero-premature-load correctness remain required. No GPU residency is inferred.

[Rolling machine status](performance-results.json) binds the current
[P01 machine results](t004-p01-results.json), exact historical campaign links and
ownership. The original T020 aggregate and completion receipt remain byte-exact:
[t020-validation.json](t020-validation.json) binds the
[archived T020 aggregate](t004-p01-evidence/pre-t004-aggregate-results.json).
Its next-task/decision fields describe the T020 completion checkpoint, not the
current queue. The new evidence manifest and explicit post-campaign archive
bindings preserve frozen task/ledger bytes without rewriting observations.

No campaign is resumed, retried, pooled or reclassified. No numeric budget,
production behavior or archive policy changed in this measurement slice.


## T004.2 Exploratory Diagnostic Checkpoint

The [new diagnostic report](t004-p07-diagnostic-results.md) records exactly16
corrected fresh GUI workers, all fidelity/runtime checks passed, and two
39,985-binding audits. With only two observations per arm/context, these are
descriptive results, separate from D001 acceptance: hidden open4094→2071ms,
firstshow85→1012ms, open+show4179→3082ms, timed-window RSS932→845MiB. Roughly80%
of lazy firstshow is read/decode. Controls/load/observer-cost limits remain
explicit; this does not establish a cause for old BIMp95. The earlier aborted
worker and separate2-process resource-mapping forensics remain preserved.

`performance-results.json` remains the hash-bound T004.1 acceptance checkpoint;
its P01/P07 unaccepted classifications are unchanged. The new diagnostic machine
results and validation are separately bound, avoiding edits to historical
validation receipts. T004.2 still owns P04/P05 comparative conversion work.
