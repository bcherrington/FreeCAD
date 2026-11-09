---
title: T004.4 workflow cost-benefit and operating-scope review
doc_type: spec
artifact_type: review
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# T004.4 Workflow Cost-Benefit And Operating Scope

This completes the D007 trigger review for P01–P08 using existing qualified evidence.
It is an explicit scoped product judgment, not a new statistical pass or final
production sign-off. All original FAIL/INCONCLUSIVE labels and frozen policy,
results and receipts remain unchanged. No new runtime samples were collected.
The companion [JSON](t0044-cost-benefit-review.json) binds every source and carries
all eleven D007 fields for every comparison, including attribution exclusions.

T004.4 resolves the operating-scope choices below; T010 confirms exact final
source, build, platform and evidence applicability. It does not inherit unanswered
T004.4 decisions. SC-005 and whole-spec acceptance remain open until those gates.
Unknown population frequency is handled by selecting workflows, never by inventing
usage weights or claiming a population-wide benefit. Scope limits are evidence
and review limits; they introduce no software clamps or functional restrictions.

## Decisions

| Change | D007 disposition | Principal cost and boundary |
| --- | --- | --- |
| P01 restore | Retain for payload-rich ordinary opens | Large payload benefit; BIM/Arch tail risk accepted in scope, no latency SLA or host-cause inference |
| P02 save | Retain for qualified serial payload-rich saves | Paired full-save benefit about 519 ms; improved p95, worse observed maximum; process peaks not retained-memory proof |
| P03 parser | Retain bounded safe parser for local benefit | Milliseconds saved in hooks; whole-open gain unproven, Draft tail risk disclosed |
| P04 direct conversion | Retain qualified direct formats | Large local conversion gain despite dispersion; no universal GUI multiplier |
| P05 fallback conversion | Retain with bounded memory accounting | At most three materialized 4096-equivalent fallback planes, one active conversion, adequate headroom; wider inventory/overlap outside approval |
| P06 restore/import guard | Adopt reviewed attribution exclusion | Shared generalized lifecycle; zero premature loads mandatory, no separate speed claim |
| P07 lazy images | Retain for hidden/subset workflow | Hidden-open/RSS gains in exchange for occasional first-show stalls; measured p95 1.843 s, maximum 1.931 s |
| P08 tree | Adopt reviewed current-effect attribution exclusion | Preserve lifetime/fallback/finish correctness; no separable current speed claim |

## Detailed Trigger Closure

### P01

**Legacy outcome and trigger.** Original overall FAIL: BIM open-p95; ArchDetail/PartDesign inconclusive. All original classifications preserved.

**Evidence and source scope.** t004-p01-results.md/json; t004-p01-validation.json. Five qualified cells, 240 workers, 20 measured pairs/cell; pinned source/build/runtime and included-file fidelity/error checks. Console-open scope, not GUI frame timing.

**Absolute and relative benefit.** Included-file primary payload median 513.92 to 26.79 ms (-94.8%); console open 1183.27 to 704.44 ms (-40.5%). Control workloads have little/no included payload; no equivalent gain claimed.

**Paired confidence and tail behavior.** Primary payload paired-ratio 95% bootstrap CI [0.05186, 0.05482], 20 measured pairs/cell. BIM open p95 2784.98 to 3347.69 ms (+562.71); ArchDetail p95 4907.17 to 5375.69 ms. No attribution to host load and no guarantee for latency-sensitive BIM/Arch opens.

**Operation frequency.** Population frequency unknown. Benefit applies per included-file-rich open; no weighted average or claim that every document benefits.

**Transient and retained memory and process peak.** Original resource gates pass. Buffered restore adds bounded temporary storage; campaign process peaks do not establish long-lived allocator behavior or retained-memory neutrality.

**Concurrency and operating scope.** Retain for ordinary interactive desktop and non-deadline batch opens within recorded fixture/source/runtime scope. Overlapping opens not measured; no concurrency multiplier or hard latency SLA approved.

**Uncertainty and applicability.** BIM tail cause unresolved; accepting observed workflow risk does not establish causality. Exact final composition/platform remains T010.

**Rationale and mitigation.** Accept large payload-path benefit with disclosed control tails. Keep current byte/error/permissions semantics; report workflow-specific regressions rather than discard on percentage alone.

**Owner and residual tasks.** local-developer owns scoped disposition; T010 confirms final source/platform applicability. A strict latency target or changed overlap pattern requires an explicit applicability review.

**Decision and production acceptance scope.** Retain; scoped D007 tradeoff accepted. No v1 pass reclassification or final production approval.

### P02

**Legacy outcome and trigger.** Primary PASS; small zero-payload controls INCONCLUSIVE. Review maximum and process-peak increases.

**Evidence and source scope.** t0043-results.md/json; t0043-validation.json; t0043-evidence/p02-statistics.json. 144 workers, three cells; byte/ZIP/failure/read-only and geometry/payload checks; qualified defined ABI.

**Absolute and relative benefit.** Primary full-save medians 2695.169 to 2113.726 ms; paired saved 519.219 ms (95% CI 434.823 to 644.572). Hook medians 1897.657 to 1338.029 ms; paired saved 513.575 ms (CI 473.478 to 598.862).

**Paired confidence and tail behavior.** Twenty measured pairs/cell. Full-save p95 4158.546 to 3214.055 ms, but maximum 5054.700 to 5193.201 ms (+138.501). PartDesign p95 112.365 to 115.386 ms; no deadline guarantee or uniform control speedup.

**Operation frequency.** Save frequency unknown; per-save gain matters when included-file payload is substantial. Zero-payload fixtures do not establish benefit.

**Transient and retained memory and process peak.** 64 KiB temporary buffer per active save in source. Median process peak primary 335.22 to 337.28 MiB, controls roughly +1.6 to +1.7 MiB; includes preparation and is not incremental retained memory.

**Concurrency and operating scope.** Retain qualified payload-rich serial saves; one active benchmark save. Overlapping saves have at least one buffer per active operation; total process memory/overlap not measured. No new concurrency or latency guarantee.

**Uncertainty and applicability.** Scalar comparator and ABI/provider qualification are explicitly scoped; final production source/build remains T010. Population and long-lived RSS unknown.

**Rationale and mitigation.** Accept about half-second paired save benefit and improved p95 despite the observed worse maximum/small process peaks. Preserve visible archive errors and byte identity.

**Owner and residual tasks.** local-developer; T010 final composition/platform. Reassess changed buffer lifetime, archive error path or materially concurrent workload, not old dispersion alone.

**Decision and production acceptance scope.** Retain; scoped D007 tradeoff accepted; production acceptance remains false.

### P03

**Legacy outcome and trigger.** Primary hooks PASS; whole-open confidence crosses zero; Draft overall legacy FAIL and higher process peaks.

**Evidence and source scope.** t0043-results.md/json; t0043-validation.json; t0043-evidence/p03-statistics.json. 240 accepted workers/five cells; safe grammar and owning-token versus bounded-view comparison, not literal historical Boost replay. Earlier 135-row verifier-scope attempt excluded, never pooled.

**Absolute and relative benefit.** BIM hook paired saved 5.924 ms (CI 4.696 to 6.803), paired ratio CI 0.884 to 0.923. Arch hook paired saved 1.780 ms (CI 1.194 to 2.522), ratio CI 0.900 to 0.951. Whole-open improvement not established.

**Paired confidence and tail behavior.** Twenty measured pairs/cell. Draft open median 400.312 to 392.163 ms, p95 803.774 to 1002.837 ms, maximum 1052.9 to 1617.7 ms. Hook is sub-ms. Accept this measured uncertainty for ordinary desktop scope, not a Draft latency SLA or host-cause explanation.

**Operation frequency.** Map-token frequency varies by document and is not a population measurement. Claim bounded local parsing benefit only, no noticeable whole-open speedup.

**Transient and retained memory and process peak.** BIM median process peak 280.30 to 281.99 MiB; Arch 266.70 to 268.67 MiB. Bounded views avoid owning token strings locally; fresh-process peaks do not prove total-memory reduction or neutrality.

**Concurrency and operating scope.** Retain qualified safe parser on recorded serial restore workloads, ordinary interactive use. Concurrent restore throughput and strict Draft response deadlines outside performance approval.

**Uncertainty and applicability.** Whole-open CIs crossing zero and Draft tail remain disclosed. Exact geometry-manifest scope is the qualified one; no universal topological-name proof. Long-lived RSS unknown.

**Rationale and mitigation.** Accept modest local benefit and safe bounded implementation with no end-to-end speed promise; preserve numeric overflow/grammar/lifetime/identity hard gates. Tail regression reports trigger workflow review.

**Owner and residual tasks.** local-developer owns decision; T010 confirms final composition/platform. New parser semantics or a strict latency requirement need affected qualification.

**Decision and production acceptance scope.** Retain; local-hook tradeoff accepted, whole-open benefit unproven; production acceptance false.

### P04

**Legacy outcome and trigger.** Original timing INCONCLUSIVE from dispersion although all timing/resource numeric thresholds pass.

**Evidence and source scope.** t004-conversion-results.md/json and t004-conversion-validation.json. Three direct-format cells among 13 qualified conversion cells; exact pixel/orientation/ownership checks and actual-interpreter equivalence limits retained.

**Absolute and relative benefit.** Primary raster medians approximately 928.30 to 92.21 ms and 769.75 to 79.59 ms; paired ratio CIs about [0.090,0.107] and [0.089,0.108]. Small raster 89.89 to 11.68 ms. Local conversion only.

**Paired confidence and tail behavior.** Twenty measured pairs/cell. Primary IQR/median 31.1% to 15.5% and 17.9% to 16.9%; small control 162.1% to 102.6%. Paired confidence supports local primary gain; original dispersion labels remain. No universal GUI multiplier or latency SLA.

**Operation frequency.** Conversion once per materialization; image count/replacement frequency population unknown. No weighted whole-document or repeated cached-show benefit claimed.

**Transient and retained memory and process peak.** All original resource gates pass for direct formats. Conversion storage/ownership qualified; fresh-process RSS does not establish allocator/GPU residency or long-lived reclamation.

**Concurrency and operating scope.** Retain qualified direct formats/dimensions on one active conversion, within recorded native conversion profile. Concurrent conversion and different formats/sizes need applicability assessment.

**Uncertainty and applicability.** Conversion microbenchmarks do not predict exact GUI frame/open time. High relative variation in a small cell is not hidden or rerun away.

**Rationale and mitigation.** Accept large absolute/local gains despite dispersion; preserve resolution, pixels, orientation, lifetime and calibration. No extra optimization solely to satisfy 10% IQR.

**Owner and residual tasks.** local-developer; T010 final source/platform/profile confirmation. New direct-format routes require fidelity and applicable evidence.

**Decision and production acceptance scope.** Retain; qualified local-conversion benefit accepted with scope limits; production acceptance false.

### P05

**Legacy outcome and trigger.** Five large fallback cases original resource FAIL; timing INCONCLUSIVE despite numeric timing passes. Five small controls pass resources.

**Evidence and source scope.** t004-conversion-results.md/json; t004-conversion-validation.json. Qualified fallback formats at 4096 and 512 dimensions, pixel/orientation/ownership checks. Many-image figures below are reviewed logical accounting, not an empirical many-image campaign.

**Absolute and relative benefit.** Large-case paired conversion savings approximately 276 to 505 ms, current conversion medians approximately 53 to 97 ms. Local materialization benefit, not whole-workflow confidence.

**Paired confidence and tail behavior.** Twenty measured pairs/cell. Across five large cells, paired-ratio 95% bootstrap CI endpoints span [0.0848, 0.2320]; this is a range across separate cell intervals, not a pooled CI. Baseline IQR/median ranges 11.6% to 90.2%, current 5.5% to 27.3%. Exact per-cell p95/max and confidence are reproduced below and in supporting_P05_large_conversion_cells. Original timing remains INCONCLUSIVE. No long-lived or many-image tail distribution measured; no guaranteed conversion deadline.

**Operation frequency.** Cost is incurred per newly materialized/replaced image, retained cost persists per materialized plane. Frequency unknown; do not assume all files contain a single image.

**Transient and retained memory and process peak.** Per 4096x4096 fallback conversion: +64 MiB temporary decoded RGBA payload. Additional retained Coin payload: indexed gray +48 MiB/plane; gray-alpha and Gray16 +32 MiB/plane; premultiplied and SVG cases +0. At N planes, gray extra is 48N MiB (32N for the two other cases); K overlapping conversions add 64K MiB temporary. At 512 these terms scale by 1/64. Source lifetime ends after conversion, allocator RSS reclamation unmeasured. GPU and old/new-copy overlap unmeasured; do not sum these with unrelated measured process peaks.

**Concurrency and operating scope.** Select a bounded review scope: serial materialization, at most three materialized 4096-equivalent fallback planes, each dimension no larger than the qualified 4096 case. This permits up to 144 MiB extra retained gray payload (96 MiB for gray-alpha/Gray16) plus a separately accounted 64 MiB active temporary. Three is an explicit local operating-scope decision, not a tested three-fallback result, a software clamp, or an assertion about typical users. Actual decoded/base/Coin/driver memory and headroom must also fit. Many-image counts above three, K>1, replacement overlap, larger dimensions and sustained allocator reuse are outside this performance approval.

**Uncertainty and applicability.** No universal RAM budget, GPU bound, population count or RSS ceiling inferred. Selected accounting scope is conditional on adequate actual memory headroom; it does not promise this fits every machine or narrow FreeCAD functionality.

**Rationale and mitigation.** Accept speed/fidelity benefit at the explicit bounded inventory; retain full resolution and calibration. Inventory images and formats before extrapolating. If scope must grow, review the accounting/headroom and collect only evidence needed for a real gap; do not force a source rewrite to the old 1 MiB trigger.

**Owner and residual tasks.** local-developer owns selected scope and headroom judgment; T010 confirms final composition and stated scope. Broader operating scope requires a separately recorded extension decision with relevant evidence, not silent approval.

**Decision and production acceptance scope.** Retain; bounded memory-for-speed tradeoff accepted by reviewed accounting. Broad many-image/overlap approval and production approval not granted.

### P06

**Legacy outcome and trigger.** Reviewed attribution exclusion, no independent speed classification.

**Evidence and source scope.** t020-measurement-review.md and t004-execution-plan.md P06 inventory; qualified T006/T020 restore/import lifecycle guard shared with P07.

**Absolute and relative benefit.** No separable speedup claim; generalized guard preserves pending images through restore/import.

**Paired confidence and tail behavior.** No independent P06 timing distribution is identifiable under the qualified common lifecycle; do not manufacture one by unsafe guard removal.

**Operation frequency.** Relevant per restore/import; population frequency unknown.

**Transient and retained memory and process peak.** No independent P06 memory saving claimed; P07 contains measured hidden/materialized logical storage.

**Concurrency and operating scope.** Retain shared qualified guard; zero premature loads required. No concurrency extension beyond qualified GUI lifecycle.

**Uncertainty and applicability.** Attribution exclusion does not remove the mandatory functional/provenance checks.

**Rationale and mitigation.** Adopt reviewed exclusion as final T004 disposition. Preserve generalized restore/import behavior and lifecycle identity; no benchmark solely to split shared effects.

**Owner and residual tasks.** local-developer; T010 checks final lifecycle composition/platform. No independent performance residual remains in T004.

**Decision and production acceptance scope.** Retain under reviewed non-identifiability exclusion; no standalone benefit or production approval.

### P07

**Legacy outcome and trigger.** Original recovery-2 overall FAIL: first-show budget, all-visible p95 and PartDesign control; other timing INCONCLUSIVE. Original 500/1000 ms limits remain diagnostic triggers.

**Evidence and source scope.** performance-recovery-2-results.md/json; t020-validation.json; t004-p07-diagnostic-results.md/json separate exploratory n=2/arm (never pooled or used as a tail acceptance sample). Twenty-pair recovery-2 tails govern this review, not the diagnostic.

**Absolute and relative benefit.** Hidden-open medians 3321.13 to 1736.84 ms; paired median saved 1726.52 ms and ratio CI [0.479,0.539]. Open+first-show medians 3390.18 to 2565.95 ms but original result INCONCLUSIVE. Repeat median 7.78 to 7.66 ms. Avoided work only for images never shown; otherwise deferred.

**Paired confidence and tail behavior.** First-show median 71.66 to 857.91 ms; p95 103.36 to 1843.09 ms; maximum 132.69 to 1930.75 ms. Open+first p95 6570.10 to 5488.32 ms. All-visible p95 11922.84 to 14979.88 ms; PartDesign median 284.79 to 318.75 ms. Accept occasional first-use stalls in selected hidden/subset workflow; neither sub-second response nor whole-workflow universal gain is promised.

**Operation frequency.** Select opens with images hidden and remaining hidden or only a subset revealed. Frequency in the user population is unknown; opening all images immediately is outside performance-benefit approval. Already-materialized repeat display does not reload.

**Transient and retained memory and process peak.** Hidden maximum RSS 921.39 to 832.16 MiB; peak Coin payload 280.04 to 186.69 MiB, decoded 93.35 MiB both. All-visible RSS 918.21 to 920.04 MiB; Coin 280.04 MiB both. Savings are scoped peak observations, not permanent savings after all images are shown. GPU residency unmeasured.

**Concurrency and operating scope.** Retain qualified three-image hidden fixture workflow on the recorded desktop profile, serial first demand. One first-show materializes one image and leaves two pending; repeat adds no load. Immediate-all-image and concurrent-demand responsiveness claims excluded. P05 format/count accounting applies separately to fallback payloads; do not combine unrelated arm deltas.

**Uncertainty and applicability.** No idle-host inference or explanation of older BIM tail. Diagnostic decomposition (~80% lazy first-show read/decode) is descriptive only. Broader platform, long-lived memory and composed final source remain T010.

**Rationale and mitigation.** Accept hidden-open speed and avoided materialization/storage for images never needed, in exchange for observed occasional first-use latency up to about 1.93 s in this sample. Keep full resolution/calibration and safe lifecycle; no newly invented 1 s global budget, background loader or placeholder change.

**Owner and residual tasks.** local-developer owns this selected experience; T010 confirms exact final profile/source and scope. Strict first-frame deadlines or broader immediate-all-visible use need an explicit applicability review.

**Decision and production acceptance scope.** Retain; scoped hidden/subset workflow tradeoff accepted. Legacy failures remain; production acceptance false.

### P08

**Legacy outcome and trigger.** Reviewed current-effect attribution exclusion; historical suppression is subsumed by correctness repairs.

**Evidence and source scope.** t0043-results.md/json P08 disposition; T014/T017/T007 source and functional receipts. Current safe fallback timer/finish flush must remain.

**Absolute and relative benefit.** No separate tree speedup claimed; historical blanket timer suppression cannot be isolated by a safe raw revert.

**Paired confidence and tail behavior.** No independent comparable timing/tail distribution; exclusion is source-reviewed rather than an unmeasured gain.

**Operation frequency.** Restore/tree-update frequency population unknown.

**Transient and retained memory and process peak.** No independent retained-memory or process-peak saving claimed. Receiver lifetime checks remain hard gates.

**Concurrency and operating scope.** Retain qualified current GUI tree lifecycle and event-loop behavior. No nested/open-depth concurrency design extension or larger-model speed promise.

**Uncertainty and applicability.** Do not treat old suppression source presence as current measurable speed benefit. B011 remains inactive, not a prerequisite to this attribution disposition.

**Rationale and mitigation.** Adopt reviewed exclusion; preserve T014 lifetime, T017 fallback/finish and T007 correctness. Resolve T004.4 attribution residual without unsafe historical counterfactual or new timing campaign.

**Owner and residual tasks.** local-developer; T010 confirms final source/platform. No remaining T004 performance residual; optional further optimization stays B011.

**Decision and production acceptance scope.** Retain qualified correctness-preserving tree path under reviewed exclusion; production acceptance false.

## P05 Qualified Single-Conversion Tails

These are separate 20-pair large cells, not a many-image or pooled sample.
All values below are milliseconds except the paired ratio interval.

| Cell | Paired ratio 95% CI | Baseline → current p95 | Baseline → current maximum |
| --- | --- | --- | --- |
| indexed-gray-4096 | [0.2081, 0.2230] | 1811.35 → 280.93 | 1913.71 → 295.14 |
| indexed-gray-alpha-4096 | [0.2176, 0.2320] | 1824.05 → 319.35 | 1906.94 → 338.08 |
| premultiplied-4096 | [0.1161, 0.1313] | 1343.58 → 105.20 | 1398.03 → 109.90 |
| gray16-4096 | [0.1469, 0.1603] | 1573.63 → 198.67 | 1588.29 → 233.35 |
| svg-4096 | [0.0848, 0.1287] | 2076.49 → 123.88 | 2139.88 → 215.60 |

## Verification And Durable Routing

[T004.4 validation](t0044-validation.json) records document/accounting/hash checks
and independent review. [Prior authority](t0044-prior-authority/manifest.json)
preserves the eight exact completion-time T004.3 authority files; historical
receipts remain meaningful as current docs advance.

Lasting scoped decisions are promoted to the [performance reference](../../reference/document-open-performance.md)
and the [review runbook](../../runbooks/freecad-performance-optimization.md).
No new global ms/MiB exchange rate, 1-second promise, image-resolution change,
background-loading design or mandatory algorithm rewrite was selected.
