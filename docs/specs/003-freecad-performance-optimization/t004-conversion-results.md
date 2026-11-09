# T004.2 image conversion results

Complete comparable measurement and disposition; **production acceptance remains outstanding**.
All original D001 thresholds and old evidence remain unchanged. Parent T004 owns
every failed/inconclusive outcome, including retained P01/P07.

P04 compares the bounded historical per-pixel direct conversion with the current
direct path. P05 compares the bounded historical fallback, preserving 1/2-component
storage, with current Qt RGBA conversion. Both retain identical lazy P07 behavior.
The prospective [plan](t004-conversion-plan.md) and
[reconciliation](t004-conversion-reconciliation.json) define measurement completion
and distinct refined fixture identities before sampling.

| Comparison / fixture | Before → after conversion median (ms) | Coin storage before → after (MiB) | D001 |
| --- | ---: | ---: | --- |
| P04 raster-a | 928.296 → 92.208 | 93.35 → 93.35 | INCONCLUSIVE |
| P04 raster-b | 769.746 → 79.594 | 93.35 → 93.35 | INCONCLUSIVE |
| P04 raster-c | 89.894 → 11.676 | 13.34 → 13.34 | INCONCLUSIVE |
| P05 indexed-gray-4096 | 405.923 → 88.569 | 16.00 → 64.00 | FAIL |
| P05 indexed-gray-alpha-4096 | 428.968 → 97.047 | 32.00 → 64.00 | FAIL |
| P05 premultiplied-4096 | 458.446 → 56.274 | 64.00 → 64.00 | FAIL |
| P05 gray16-4096 | 330.611 → 53.373 | 32.00 → 64.00 | FAIL |
| P05 svg-4096 | 580.328 → 61.746 | 64.00 → 64.00 | FAIL |
| P05 indexed-gray-512 | 6.010 → 1.618 | 0.25 → 1.00 | INCONCLUSIVE |
| P05 indexed-gray-alpha-512 | 8.184 → 1.717 | 0.50 → 1.00 | INCONCLUSIVE |
| P05 premultiplied-512 | 16.056 → 3.488 | 1.00 → 1.00 | INCONCLUSIVE |
| P05 gray16-512 | 7.669 → 1.647 | 0.50 → 1.00 | INCONCLUSIVE |
| P05 svg-512 | 9.723 → 1.335 | 1.00 → 1.00 | INCONCLUSIVE |

Each cell contains one diagnostic pair, three discarded warm-up pairs and 20
measured pairs; alternating AB/BA, one fresh process and conversion per arm.
All observations and phase labels are retained. No retries, replacements,
exclusions or pooling occurred. Runtime mapping/source/fixture/output identity
and full pixel/alpha/orientation/ownership checks passed for every worker.

Logical Coin/decoded storage and process RSS are different quantities. One- and
two-component historical grayscale fields become four components in the candidate;
these increases must remain resource failures wherever they exceed D001.
Conversion speed does not waive a storage failure.

Every 4096 case adds **64 MiB of peak logical decoded storage** for the Qt RGBA
temporary, exceeding the unchanged 1 MiB allowance; process RSS also fails in
all five large cells. Indexed gray adds 48 MiB of Coin storage, while gray-alpha
and Gray16 add 32 MiB. Premultiplied/SVG Coin storage stays unchanged, but their
temporary/RSS gates still fail. All 512 controls pass resource limits.

All timing cells meet their numeric gain/non-regression thresholds, but exceed
D001's variability limit and therefore remain formally inconclusive. These are
complete dispositions, not performance acceptance.

The aggregate host snapshots occur between workers and add no collector inside
the measured conversion. Their observed cost and load ranges are published in
the evidence summaries. They do not establish causation for the old BIM p95.

## GUI lifecycle and bottlenecks

The immutable T020 campaigns and the separately completed
[P07 diagnostic](t004-p07-diagnostic-results.md) retain the GUI opening, hidden
first-show, repeat-show, visibility/pending/materialization and resource evidence.
No primitive conversion timing is relabeled as a GUI first-frame improvement.
The corrected P07 diagnostic found hidden opening 4.09→2.07 s and about 87 MiB
lower peak RSS, with first reveal 85→1012 ms (~80% read/decode). Two observations
per arm/context are exploratory and do not establish acceptance. P07 stays retained
and unaccepted under unchanged first-show budgets.

## Evidence and remaining work

All machine decisions are in [results](t004-conversion-results.json); the
[evidence manifest](t004-conversion-evidence/evidence-manifest.json) binds sanitized
observations, qualification, host summaries and provenance. Private payloads,
locators, process IDs, absolute timelines and loaded-map paths remain local.

T004.2 records complete measurement/dispositions. Parent T004 stays in progress
for all numeric acceptance residuals; next delivery slice is T004.3 parser/tree/save
comparison or reviewed exclusion. Final integration/desktop/platform applicability
remains T010. No production conversion behavior, calibration, budget or old sample
was changed. The single optimized pair was reused in place; no full build/cache
copies were added.

P04 passed 148 native cases per arm. P05 passed the current candidate’s full 148
cases; its reconstructed historical comparator passed 5 shared direct/null, 24
lifecycle and 12 render/calibration cases, plus full-image qualification for every
one of the ten fallback cells. The historical comparator is a bounded measurement
arm, not a production implementation passing the complete current format suite.

P04 actual-controller identity was independently verified from the explicit
operator launch and live executable/argv/start identity while collection ran.
That reviewed provenance equivalence changes no sample or criterion. P05 adds
automatic exact interpreter/pidfd preflight and a hash-bound activated environment.
The original unlaunched P04 draft and earlier qualification remain preserved.
