---
title: T004.3 save parser and tree evidence plan
doc_type: spec
artifact_type: execution-plan
status: selected-for-construction-and-qualification
owner: local-developer
last_reviewed: 2026-11-08
---

# T004.3 — Per-Change Evidence Under D007

Bind acceptance-policy-v2.md/json and this review rubric before any comparative
samples. The original benchmark-plan/protocol/assessor remain unchanged v1
measurement/statistical evidence; D007 product direction and T004.4 closure are
separate. Correctness/provenance/coverage remain hard gates. Do not repeat prior
campaigns, alter budgets/data, add resolution trade-offs or infer host causation.

## Construction And Qualification

Reuse the sole existing optimized pair in place, after explicitly retiring P05
native replay and retaining refs, raw evidence, controller sources and receipts.
Start from the qualified retained-lazy common source, not the moving root worktree.
Keep dependencies/profile/renderer requirements and all source safety identical.
Any shared diagnostic instrumentation is opt-in, absent from normal builds.

P02 varies only successful scalar byte-copy versus current 64 KiB buffered save.
Both arms retain the create/read/write checks and current restore, notifications,
permissions, later-entry behavior and all other optimizations. Instrument the
actual SaveDocFile scope once, with successful/failed/empty/active counts; do not
add per-byte instrumentation that unfairly penalizes the scalar arm. Save only a
private worker copy. Verify payload bytes/declarations and the explicitly scoped persistent-object,
geometry and reference oracle below after save/reopen; original fixtures remain
byte-identical. Python proxy internals and document-level metadata are outside
this oracle; proxy class identity remains checked.

P03 first requires positive actual restored mapping records/token buffers in both
original primary documents. Count native ElementMap restore calls, parsed entries,
child/SID token buffers and failures, not XML appearances. If covered, compare
allocation-owning split tokens with string-view tokens while sharing checked numeric
grammar/ranges/ownership/hasher/alias behavior. Do not restore unsafe strtol or
remove T015/T016 safeguards. If original fixtures do not execute this mechanism,
record a coverage failure and independently reviewed exclusion/routing to T004.4;
claim no parser speedup and do not silently substitute a synthetic workload.

P08 disposition: exclude independent timing of the retained current effect. Source
review confirms that T017's required owned fallback timer and finish flush subsume
the historical restore-time timer suppression. A raw reversal would also remove
T014 receiver-lifetime safety or strand direct/import/error restore paths. Preserve
T014/T017/T007 behavior; claim no P08 speedup and run no new P08 campaign. The sole
residual disposition owner is T004.4/local-developer. A future nested application
open-depth redesign is an inactive B011 candidate, not this retained comparison.
Independent reviewer: p01_instrumentation_review; origin comparison: c1722e4b2c5ab7019cd3fdce08eb7199459de859 to
00dd158de837fee43074db8b28979e30e4210d11. Later correctness commits
3587399d1b and da90da6f93 govern the current behavior.

## Bounded Coverage And Hard-Gate Checks

Before sampling, run one fresh deep console qualification per distinct required document
(private primary, ProjectTest, PartDesignExample, BIMExample, ArchDetail,
EngineBlock, draft_test_objects: seven documents). These runs establish actual
save/parser coverage, exact normalized model and payload identities, completion
recompute count and runtime maps. Their timing values are qualification-only and
are not accepted/published as comparative performance. Deep semantic qualification has
a separate 180-second outer / 175-second native bound; samples retain the original
60-second outer / 55-second native bound. The initial 55-second qualification
limits proved insufficient for the expanded semantic oracle; preserve those
timeouts and do not reinterpret them as performance failures. Any corrected qualification
attempt is retained with its failure/disposition, not pooled into performance.

Qualify the matched arms with focused archive/save failure and parser/map/reference
regressions as affected. Prove actual target execution and absence of failures.
Record build commands/flags/source/object/runtime equivalence and final binaries.
All snapshots, input bytes, worker/helper/tool/environment identities and private
cache trees must be frozen/audited before and after each complete campaign.

## Sampling And D007 Review Rubric

For identifiable qualified changes, retain every originally declared primary and
control context. Use original 24 alternating AB/BA fresh-process pairs per cell:
one diagnostic, three discarded warm-ups, 20 measured pairs. Preserve 60-second/
256 KiB worker and 1200-second cell bounds; stop on ownership, provenance or
correctness failure. No retries, replacements, posthoc exclusions, pooling or
adaptive sample addition. Zero mechanism coverage requires a pre-sample explicit
inapplicability/exclusion decision; other document/resource metrics remain useful.

Use the qualified isolated controller/namespace and frozen allowlisted activated
Pixi environment. Verify actual interpreter/pidfd cleanup and exact worker argv;
validate D007 policy/rubric hashes before artifacts. Collect low-cost aggregate
host CPU/load/memory/PSI snapshots between workers, outside measured operations.
No concurrent build/test/worker, cache drops, power changes or killing user apps.

Report each per-change absolute/relative median, p95, paired confidence/savings,
dispersion, meaningful work counts, full document endpoint and peak process RSS/
logical image bytes. Console workflows require zero image materialization; do not
invent GUI or GPU timing. Tree comparisons need the qualified actual GUI endpoint.
Keep original v1 metric classifications as diagnostics, never as D007 release
approval. D007 disposition records benefit, uncertainty/tails, frequency, temporary
versus retained memory, concurrency/scope, implementation cost, rationale and
mitigation. Unknown workflow fields remain explicit T004.4 work; T010 confirms
final composition/platform applicability.

## Completion Boundary

T004.3 completes per-change measured evidence or source/coverage-supported reviewed
exclusions for P02/P03/P08, with exact provenance and one residual destination for
each unresolved comparison. It does not complete T004 aggregate or T004.4 operating
scope, certify production approval, or reuse combined open gains as independent
algorithm improvements. Stop new sampling after the selected finite campaigns.

## Cross-Save Fidelity Oracle (Prospective Qualification Refinement)

The original open-only BRep hashes remain unchanged. Exploratory cross-save
qualification found that the same kernel's BRep import/export changes direction
components independently of P02; raw exported text hashes cannot establish
cross-save identity. Preserve these failed qualification attempts as diagnostic
evidence, never performance observations or product regressions.

Before comparative samples, requalify all seven documents using
`document_performance_fidelity.py`, whose implementation and tests are frozen with
the controller. For CASCADE Topology V1 (Matra-Datavision header), preserve line,
section, token order/count and every numeric value except the following explicitly
recognized unit-basis slots: Curves type 1 / 7 tokens, offsets 4–6; Curves type 3 /
15 tokens, offsets 4–12; Surfaces type 1 / 13 tokens, offsets 4–12; and the first
three columns of each kind-1 Locations 3×4 matrix. Offsets are zero-based.
Unknown kinds/lengths, origins/translations, radii, knots, degrees, topology/count/
index fields remain exact. Equal signed zero is allowed. Reject non-finite values.
Permitted component differences are at most 2e-15 absolute; pre/post unit norms
and orthogonality are within 1e-12, with determinant magnitude within 1e-12 of one
and unchanged handedness. This allowance concerns unit-direction serialization,
not model-space distances or a general numeric tolerance.

Independently preserve shape type/null/valid and exact topology counts for every
persistent PartShape property and referenced geometric subshape. Secondary bbox
and center coordinates and length (mm), area (mm²), volume (mm³) use absolute
1e-9 in the named unit and relative 1e-12; these never override token/basis checks.
The existing independent archive-based object/type/proxy-class/image-calibration/
placement/payload and LinkSub checks remain. Compare all other persistent object
property schemas, type/status flags, and exact decompressed member-name/hash sets
from dumpPropertyContent(Compression=0), including ordinary links/lists/group
relationships and scalar/list/custom properties. Ignore ZIP timestamps only;
limit each property to 128 unique members and 128 MiB compressed/decompressed.
PartShape and FileIncluded properties use their separate geometry/payload oracles.
Proxy's class identity is checked; persistent Python Proxy internals and document
metadata are explicitly outside this scoped oracle, with final applicability in
T010. No full-state dumps/BRep strings enter public evidence or final diagnostics.

P02 RSS is the process-lifetime high-water through save, including preceding
fidelity preparation. It cannot establish save-incremental or retained RAM.
The open high-water is captured before fidelity checks. Qualification timings are
never promoted to comparative results. No comparative source/policy freeze or
performance samples have occurred during oracle construction.


After deep qualification establishes original-to-reopened semantic fidelity,
freeze both exact raw state identities separately. Each P02 sample must match the
qualified original state, exact persistent-property identity, save payload bytes,
and the qualified reopened state/property identities. It need not repeat the
expensive grammar/invariant proof: no new floating tolerance is applied during
sampling. P03 samples run open/recompute and the original-state/persistent-property
checks only, with no unrelated save/reopen. This keeps the measured change and
workflow endpoints separate. Full-arm qualification still checks save compatibility
where required. All qualifications precede the immutable campaign freeze.


P03 has its own explicit open-only mode qualification on each arm for all five
parser cells; freeze those actual identity dictionaries, including absent save/
reopened fields. Do not derive P03 contexts by silently editing full-save records.
Aggregate persistent dumps are bounded to 100,000 properties, 100,000 members and
1 GiB decompressed per document, in addition to the per-property caps. These are
trusted checksum-pinned fixtures; Python checks limits after the native serializer
returns and does not claim to sandbox that allocation. Freeze observed property/
member/byte counts and whole-worker peak RSS in qualification before sampling.


### Corrected Campaign After CAD Scope Review

The first P03 campaign (freeze
6a5cd15b3ba908563faff463a530d1ade94f7b8f0f2073e3aad7c2a52f903bdb)
was aborted after 135 completed observations when CAD review found an exact
sample-binding gap: the old model snapshot hashes the conventional Shape property,
while PartDesignExample also has seven nonempty AddSubShape properties. Deep
qualification covered them semantically, but per-run identities did not. This is
an ineligible verifier-scope attempt, not observed corruption or a performance
outcome. Preserve all observations, abort reason, source capsules and the passing
input/runtime audit at abort. No timing results informed this correction.

The corrected worker adds exact raw geometry-manifest identities for every
PartShape property (including additional and null properties) and every checked
referenced subshape, separately for original and reopened states. Manifests bind
sorted property/reference keys to opaque BRep hashes/null markers. The existing
semantic geometry oracle remains unchanged; it qualifies cross-save normalization,
while every accepted sample must match the respective exact qualified manifests.
Requalify both arms and rerun the complete fixed 240-worker P03 campaign under a
new freeze and distinct artifacts/caches. Never pool, retry, replace or reuse any
aborted observation. This new campaign corrects fidelity verification only; it is
not an adaptive sample addition to clear a numeric outcome. P02 uses the corrected
manifest binding from its first campaign. Require independent CAD and performance
review of this correction before the new samples.
