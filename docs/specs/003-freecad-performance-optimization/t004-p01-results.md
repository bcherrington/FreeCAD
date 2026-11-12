---
title: P01 comparable restore benchmark results
doc_type: spec
artifact_type: verification
status: measured
owner: local-developer
last_reviewed: 2026-11-08
---

# P01 Comparable Restore Benchmark Results

Five-cell measurement is complete. **P01 is NOT ACCEPTED overall (D001 FAIL).** All 240 fresh-process observations completed, with twenty measured pairs per cell and no worker failures, retries, exclusions or pooling. P07 remains retained and unaccepted; T004 overall remains incomplete.

| Fixture | Payload median ms, scalar → buffered | Open median ms, scalar → buffered | Open p95 ms, scalar → buffered | Cell outcome |
| --- | --- | --- | --- | --- |
| gridfinity-drawer-v1.3 | 513.92 → 26.79 | 1183.27 → 704.44 | 1246.86 → 736.03 | pass |
| BIMExample | 0.13 → 0.13 | 2470.03 → 2446.85 | 2784.98 → 3347.69 | fail |
| ArchDetail | 0.19 → 0.16 | 2907.27 → 2955.14 | 4907.17 → 5375.69 | inconclusive |
| EngineBlock | not applicable (D006) | 314.60 → 314.04 | 338.66 → 341.63 | pass |
| PartDesignExample | not applicable (D006) | 174.77 → 176.88 | 265.06 → 258.98 | inconclusive |

## Findings And Acceptance Residual

The checksum-pinned primary passes every D001 metric. Its payload median falls
from 513.92 to 26.79 ms (94.8% lower), and complete console open falls from
1183.27 to 704.44 ms (40.5% lower). This compares the reconstructed safety-preserving
scalar baseline with buffered copy, not unsafe historical binaries or GUI frames.

BIMExample fails the open p95 non-regression rule: 2784.98 to 3347.69 ms,
exceeding the allowed 10%/5 ms increase. Its payload timing is also inconclusive
(IQR/median 0.192/0.111). ArchDetail's payload and open timing are inconclusive;
PartDesignExample's open timing is inconclusive. Their numeric limits otherwise
pass, but the predeclared 0.10 dispersion limit prevents acceptance. EngineBlock
passes its applicable gates. All resource metrics pass in all five cells.

The control tail's cause is unestablished. In particular, a sub-millisecond copy
measurement does not explain the much larger BIM tail. Retain the observed
failure/inconclusive classifications without claiming a causal regression or
retrying this campaign. Active parent T004 (local-developer) owns P01's remaining
acceptance/evidence-quality disposition; T004.1 records the completed comparison.
P07 remediation remains T004.2 and its original outcomes remain unchanged.

## Measurement And Provenance

The payload endpoint stops after copy/read/write/close checks and before permissions/notifications. Console open includes at most one required recompute; frozen completion counts are primary 0, BIMExample 3, ArchDetail 2, EngineBlock 0 and PartDesignExample 0. Independent archive bytes, permissions, full object/proxy/reference identity and exact native counters qualify every observation.

D006 retains all five cells and marks only the two structural-zero payload metrics not_applicable. Their open, RSS and image-resource rules still apply unchanged. The three payload-applicable fixtures contain 57,246,043, 519 and 5,487 bytes respectively.

Both 39,805-hash audits and live 510-package checks pass. Both arms passed 31 included/probe and 26 Reader cases, and all ten final runtime qualification contexts passed. A shared frozen SQL parser removes the nondeterministic generated-code confound; only the reviewed three native outputs differ. Earlier incomplete-runtime, fault-harness and rejected prestart candidates remain ineligible diagnostics.

This is an ordinary warmed-filesystem desktop study. Host power/load observations do not establish an idle or dedicated machine. D001 median/p95/dispersion rules are applied without reinterpreting noisy failures. Process RSS and logical image counters retain their stated scope; no GPU-residency claim.

[Machine results](t004-p01-results.json) bind exact raw observations, contexts, assessments and source/build/controller/audit hashes. [Pre-sample review](t004-p01-measurement-review.md) explains construction, correction history and limits.
