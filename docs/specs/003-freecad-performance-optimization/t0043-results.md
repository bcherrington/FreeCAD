---
title: T004.3 parser/save evidence and tree disposition
doc_type: spec
artifact_type: verification-report
status: measurement-complete
owner: local-developer
last_reviewed: 2026-11-08
---

# T004.3 — Parser/Save Evidence and Tree Disposition

P02/P03 complete their scoped core-console comparisons. P08 has a source-reviewed
attribution exclusion. This completes measurement/disposition for T004.3; D007
workflow cost-benefit and operating scope remain T004.4, with final applicability
in T010. No production approval or GUI/GPU timing is claimed.

[Machine results](t0043-results.json), [evidence manifest](t0043-evidence/manifest.json)
and [validation receipt](t0043-validation.json) bind the accepted observations. The
[execution plan](t0043-execution-plan.md) and D007 policy/rubric were frozen before
each accepted campaign. Original v1 outcomes remain diagnostics and are preserved.

## Timing Results

Values are milliseconds. Confidence intervals resample paired savings with the
original fixed bootstrap seed/count; a positive saving favors the current change.
Hook and whole-document endpoints are separate. Medians are independent summaries;
paired median savings need not equal the difference between those medians.

| Change / fixture | Hook median baseline → current | Paired hook saving [95% CI] | Whole-document median baseline → current | Whole-document p95 baseline → current | Legacy cell outcome |
| --- | ---: | ---: | ---: | ---: | --- |
| P03 / BIMExample | 59.535 → 54.150 | 5.924 [4.696, 6.803] | 2364.860 → 2378.583 | 2509.083 → 2480.610 | PASS |
| P03 / ArchDetail | 25.014 → 23.192 | 1.780 [1.194, 2.522] | 2749.586 → 2776.761 | 2997.501 → 2953.043 | PASS |
| P03 / EngineBlock | 0.412 → 0.368 | 0.041 [0.025, 0.061] | 299.148 → 301.918 | 317.347 → 322.726 | PASS |
| P03 / PartDesignExample | 2.342 → 2.016 | 0.316 [0.221, 0.376] | 166.915 → 165.715 | 175.978 → 174.575 | PASS |
| P03 / draft_test_objects | 0.377 → 0.341 | 0.039 [0.025, 0.072] | 400.312 → 392.163 | 803.774 → 1002.837 | FAIL |
| P02 / gridfinity-drawer-v1.3 | 1897.657 → 1338.029 | 513.575 [473.478, 598.862] | 2695.169 → 2113.726 | 4158.546 → 3214.055 | PASS |
| P02 / ProjectTest | N/A: zero payload/hooks | N/A | 1.419 → 1.427 | 1.727 → 1.598 | INCONCLUSIVE |
| P02 / PartDesignExample | N/A: zero payload/hooks | N/A | 50.909 → 49.987 | 112.365 → 115.386 | INCONCLUSIVE |

The two primary P03 parser hooks pass the original statistical thresholds. Their
local allocation/token-storage benefit does not establish a whole-open improvement:
whole-open paired savings confidence intervals cross zero for both primaries.
Draft remains an overall legacy failure: its tiny hook is inconclusive from high
dispersion and its whole-open p95 rises from about 804 to 1,003 ms. D007 requires
reviewing that trigger; this report does not rename it a pass or infer host causation.

## Resources and Cost-Benefit Boundary

RSS values below are MiB. The process high-water is not retained-memory evidence.

| Change / fixture | Median RSS baseline → current | Maximum RSS baseline → current |
| --- | ---: | ---: |
| P03 / BIMExample | 280.30 → 281.99 | 280.51 → 282.28 |
| P03 / ArchDetail | 266.70 → 268.67 | 267.03 → 268.84 |
| P03 / EngineBlock | 160.54 → 161.94 | 161.23 → 162.54 |
| P03 / PartDesignExample | 142.64 → 144.28 | 142.85 → 144.58 |
| P03 / draft_test_objects | 163.64 → 165.22 | 164.50 → 165.89 |
| P02 / gridfinity-drawer-v1.3 | 335.22 → 337.28 | 336.39 → 337.95 |
| P02 / ProjectTest | 77.06 → 78.65 | 77.16 → 78.76 |
| P02 / PartDesignExample | 142.21 → 143.93 | 142.34 → 144.18 |

P03 captures the process-lifetime high-water at completed open/recompute, before
fidelity work. P02 captures it through save, including preceding fidelity preparation;
it is neither save-incremental nor retained RAM. Primary P03 median RSS rises about
1.7–2.0 MiB, so no memory-neutral claim is supported. All console logical decoded/
Coin image counters remain zero. P02 has a source-known 64 KiB copy buffer per active
save call; process high-water cannot isolate that buffer.

D007 review must weigh absolute savings, uncertainty/tails, operation frequency,
temporary/retained memory, concurrency, implementation cost and operating scope.
This evidence does not resolve unknown user frequency or concurrency. Original
numeric vetoes are review triggers; mandatory fidelity/provenance/coverage remain
independent hard gates. T004.4 owns the final retained-change dispositions. The P02 primary whole-save maximum
also rises from 5,054.700 to 5,193.201 ms despite better median/p95; review that
tail separately rather than inferring its cause or relabeling the outcome.

## Workloads, Attribution and Integrity

Accepted P03 has five cells / 240 workers; P02 has three cells / 144 workers. Each
cell uses 24 alternating AB/BA pairs: one diagnostic, three warm-ups, twenty measured.
Every worker is fresh. Bounds remain 55 seconds native / 60 seconds outer, 256 KiB
stdout/stderr and result JSON, and 1,200 seconds per cell. Accepted campaigns have
zero worker failures, retries, replacements or pooled observations. Both pre/post
input/runtime audits pass; source/policy capsules and all raw records are retained.

P03 measures outermost native inner-restore scopes plus completed console open
and one required recompute, excluding startup/fidelity/hash checks. It varies minimal
owning token storage versus string views while retaining checked numeric grammar,
ownership and current safety behavior. This is not literal historical Boost/strtol
replay or a measurement of every historical parser change.

P02 measures included-file SaveDocFile scopes and the full console saveAs call,
ending after archive completion. Only scalar byte copy versus 64 KiB buffering varies;
create/read/write checks, Restore and all other changes are shared. No per-byte trace
penalizes the scalar arm. The private primary contains three included files totalling
57,246,043 bytes. ProjectTest and PartDesignExample have no included payload or save
hooks: the hook timing is prospectively N/A, while full-save/resource metrics apply.

The matched Linux optimized builds reuse one pair with the same canonical prefix,
locked dependencies and opt-in shared tracing. Exact object/link-input/command/ABI
proofs restrict App differences to the intended translation unit. GUI runtime bytes
are equal; any linked-file difference is non-allocated debug metadata. P02 adds only
the expected scalar get(char&) implementation import from the already-bound stdc++
provider; public defined ABI is unchanged. Detailed provider/locator receipts are private.

All original/reopened samples match their exact qualified geometry manifests for
every PartShape property, including additional/null shapes, and checked geometric
references. Persistent object schemas/values, payload bytes/declarations and calibration
also match. Cross-save qualification permits only the declared narrow unit-direction
serialization differences, with exact protected values/topology and independent
geometry invariants. Proxy internal state and document metadata are outside this
oracle; universal topological-name equivalence beyond checked references/regressions
is not claimed. Native regressions pass 91 cases per arm for each comparison; twenty
Python cases pass in both normal and optimized modes.

[Host summaries](t0043-evidence/p03-host.json) and
[save host summaries](t0043-evidence/p02-host.json) derive from aggregate snapshots
between workers. Their intervals include startup, fidelity and controller work. They
do not establish the cause of an older or current timing tail. No process lists,
private fixture locators, model values, credentials or geometry are published.

## P08 Reviewed Exclusion

No independent timing claim is assigned to the retained current P08 effect. T017’s
required fallback timer and finish flush subsume the original blanket suppression;
a raw reversal would remove T014 receiver safety or strand direct/import/error paths.
Preserve T014/T017/T007 behavior. The sole residual disposition owner is T004.4 /
local-developer. A future nested application-open-depth design remains an inactive
B011 candidate, with no new optimization or campaign selected.

## Failed Attempts and Preservation

Independent CAD review found that the first P03 sample identity omitted additional
PartShape properties, including seven nonempty AddSubShape values in a control. The
owned controller was interrupted through pidfd SIGINT and its cleanup path; 135
completed observations and partial output remain ineligible, with a passing input/
runtime audit at abort and exact source capsules. No observed geometry corruption
or timing result motivated the correction. No old observation is reused or pooled.

The corrected all-property manifest was independently reviewed, both arms were
requalified, and a complete new 240-worker campaign ran under a distinct freeze.
An immutable disposition binds old hashes/sizes and explicit non-use. Earlier
construction failures (output namespace, raw BRep oracle, null/compound handling
and deep-verifier timeout) remain private diagnostic attempts, not performance data.
Deep semantic qualification has separate 175/180-second bounds; AST/source review
proves the manifest addition did not change the seven-document semantic oracle.

Completed P03 native replay was explicitly retired before P02 reused the pair.
Protected source refs and full evidence remain; rebuilding later requires new
qualification and cannot recreate the original binary identity automatically. Normal
build/debug was not retired or used for these optimized comparisons. D007’s earlier
authority snapshot is preserved in [t0043-prior-authority](t0043-prior-authority/README.md);
prior policy, protocol, receipts and numeric classifications are unchanged.
