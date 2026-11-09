---
title: T004.2 direct and fallback conversion comparison plan
doc_type: spec
artifact_type: execution-plan
status: selected-for-construction-and-qualification
owner: local-developer
last_reviewed: 2026-11-08
---

# T004.2 Conversion Comparison

Complete the measurement/disposition slice for P04/P05, consuming the separately
completed P07 diagnostic and immutable T020 GUI lifecycle evidence. All D001
thresholds, prior samples and assessors remain unchanged. This plan adds no P07
acceptance campaign or product latency exception. Parent T004 retains all numeric
acceptance residuals. T004.2 completion means complete comparable evidence and
reviewed dispositions, as for T004.1; it does not mean production acceptance.
This dated plan prospectively supersedes only the T004.2 completion prohibition
in t004-execution-plan.md lines 27–31. That older plan is immutable historical
construction evidence; all of its numeric acceptance rules remain in force.
T004 and T010 still cannot complete while acceptance residuals remain. The
reconciliation receipt binds the old plan and the distinct refined recipe IDs.

## Construction

Reuse the existing optimized worktree pair in place. Before replacing its source
or outputs, retire P07 native replay explicitly, preserving source refs, immutable
raw/results/protocol/context/qualification manifests. No full build/cache copies.
Use the final lazy diagnostic source as common anchor (including shared trace),
add the identical opt-in Linux conversion executable to both arms, and restore
identical retained lazy image-plane behavior. No normal production change.

P04 baseline replaces only the recognized direct 32-bit branch with the historical
QColor per-pixel RGBA loop, retaining current safe extents/arithmetic and the
current fallback in both arms. Four-component ownership/orientation remains equal.

P05 baseline retains the current direct path, replacing only the fallback with the
historical QColor loop/component selection. Preserve table-based 1/2-component
gray/gray-alpha storage and actual source stride/depth inference for the declared
valid formats. Keep shared safe extent/allocation checks. Unsupported inferred
component counts fail safely; do not recreate unsafe behavior. The candidate's
universal RGBA allocation may exceed D001 resource limits: record that outcome.
If a fixture cannot preserve fidelity, stop its qualification and obtain a
reviewed exclusion with parent T004 owning the residual; do not measure an invalid
baseline or drop coverage silently.

## Fixture Materialization

P04 uses the three original hash-selected PNG payloads from the checksum-pinned
private archive, matching D001 hashes/sizes/dimensions. Select by SHA-256 with
32 MiB bounded reads and validate the enclosing archive before reading entries.
Never publish payloads, names, locators, pixels or screenshots.

P05 materializes all five recipes at both 512 and 4096. The plan's previously
unmaterialized recipes require these exact source-level details before timing:

- Indexed grayscale: 256-entry opaque gray palette, pixel index `(x+3*y)%256`.
- Indexed grayscale-alpha: same indices, palette gray `i`, alpha `(13*i)%256`.
  A 256-entry palette cannot independently encode all 65,536 gray/alpha pairs
  implied by the general color formula; this explicit palette specialization
  refines that ambiguity without changing dimensions or removing transparency.
- Premultiplied: original r/g/b/a formulas, setPixelColor into ARGB32_Premultiplied.
- Gray16: original gray formula expanded by multiplication by 257 into Grayscale16.
- SVG: deterministic asymmetric rectangles/circle/triangle, painted into
  ARGB32_Premultiplied (therefore actually exercising the fallback).

The native generator source fixes corner witnesses and SVG bytes; freeze its hash,
recipe IDs, actual format/stride/color space/palette/pixel hashes and dimensions
from both-arm qualification before samples. These details complete the planned
recipes; do not replace benchmark-plan.json or its original hashes. Qualification
must confirm historical components 1 (indexed gray), 2 (indexed gray-alpha and
Gray16), 4 (premultiplied/SVG) versus candidate RGBA4.

## Endpoints And Checks

One fresh isolated process per observation, one conversion. Decode/generation,
source hashing, singleton initialization and validation are outside `convert_ms`.
Time entry into actual BitmapFactory conversion through complete owned Coin field.
Full-image QColor reference verifies RGBA/alpha/orientation with the existing
maximum one-level quantization tolerance. Mutate/destroy the original QImage and
recheck output hash. Require correct trace count, balanced storage and exact runtime
mapping/binary/source/fixture identity. No output images or pixel arrays published.

Record process high-water RSS at conversion completion (startup/generator included,
later fidelity excluded), pre-conversion high-water, logical decoded peak including
Qt fallback temporary, Coin peak, and independently reported setup/decode duration.
RSS is not an isolated allocation delta or GPU memory. No GUI first-frame timing
is inferred from this primitive; T020/P07 results remain separately identified.
Focused direct/fallback/render/lifecycle qualification establishes applicability;
final integrated-source/platform acceptance remains T010.

## Sampling And Disposition

Freeze exact commits, sole production delta, generated inputs/output hashes,
binary/build/dependency/toolchain/runtime identities, private input bindings,
controller/worker/assessor and package environment. Audit immutable bindings and
clean source/dependencies before and after each campaign. P04 then P05: no parallel
build/test/worker. Rebuild the baseline in place between comparisons only after
closing and explicitly retiring P04 replay, preserving its source refs/evidence.

All 13 cells: P04 two primary payloads plus third control; P05 five large primaries
plus five small controls. Each cell uses original D001 sequence: first diagnostic
pair, three discarded warm-up pairs, 20 measured pairs, alternating AB/BA, fresh
process per arm, 60-second/256 KiB worker limits and 1200-second cell limit. Stop
on first worker/provenance/correctness failure. No retries, exclusions or pooling.
Use original assess_metric and existing fixed bootstrap/dispersion/resource rules.
Complete a cell even if numeric performance fails; report pass/fail/inconclusive.

For low observer overhead, record aggregate /proc CPU/load/memory/PSI snapshots
between workers only, not the expensive process-tree collector during conversion.
Record sampling cost and host limitations. No cold-cache or idle-host claim, global
power changes, application termination or posthoc outlier removal. Prior BIM p95
host attribution remains unproved.

Publish sanitized per-observation evidence, bindings, assessments and reviewed
acceptance dispositions. T004 owns all failures/inconclusive results; budgets stay
unchanged. Advance to T004.3 only after this slice is fully recorded and reviewed.
