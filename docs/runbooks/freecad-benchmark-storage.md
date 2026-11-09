---
title: Retain and reuse FreeCAD benchmark builds
doc_type: runbook
status: current
owner: local-developer
last_reviewed: 2026-11-08
applies_to:
  - Local FreeCAD benchmark campaigns
  - Spec 003 build and evidence storage
---

# Retain and reuse FreeCAD benchmark builds

This runbook governs local benchmark storage. Each campaign's frozen protocol,
receipts and source/output manifests govern its measurement provenance. Deleting
rebuildable files does not change a measured result or grant performance acceptance.

## Retention

Keep raw observations, failed attempts, qualifications, logs, controllers, freezes,
source/dependency identities, output manifests, audit receipts and assessments.
Protect private fixture locators and raw model data with their existing permissions.
Keep named Git refs for exact benchmark revisions. Do not overwrite old receipts to
make them describe a newer build.

Compiler caches and obsolete build outputs are disposable after a campaign ends.
Retiring them makes immediate native replay and live output auditing unavailable;
record that limitation explicitly. Reconstructing a build later does not recreate
its original qualified binary identity automatically.

Before retirement:

1. Resolve exact owned directories and reject symlinked or unexpected targets.
2. Check that no active runtime, compiler or test uses those targets.
3. Verify the outputs against the campaign manifests and retain source refs.
4. Snapshot hashes of evidence that must survive, then record the target sizes.
5. Remove only the selected outputs/caches; verify retained evidence and reusable
   outputs afterwards. Record dated results and any unavailable replay capability.

Normal `build/debug` supports development and can contain unrelated verification
artifacts. Do not retire it as part of a benchmark campaign's automatic cleanup.

## Reuse and storage budget

Keep one qualified matched build pair as a seed. While qualifying its replacement,
allow one additional pair; retire obsolete outputs after the replacement is
qualified and original evidence is retained. Do not accumulate a complete pair for
every task, failed preparation or Python-only probe revision.

Reuse source checkouts and configured outputs where their provenance is known.
Before writing into an old build, record its retirement from the old campaign,
retain its manifests/refs and establish the next exact source/dependency identity.
Run the required incremental compilation/relink and affected correctness checks,
then create new qualification/build receipts and a new freeze. Never edit Ninja
receipts or describe a reused binary as newly compiled.

Use independent writable outputs for compared arms. Do not hard-link writable
objects, libraries or Ninja state across arms or into preserved evidence trees.
If copied outputs have absolute configured paths, use the reviewed namespace
wrapper and canonical prefix; moving a build directory alone does not qualify it.

Keep the normal compiler cache shared during build preparation by selecting its
`CCACHE_DIR` explicitly in the build subprocess. Keep runtime Mesa/XDG caches
separate according to the measurement protocol. Copying the entire compiler cache
into each arm's XDG directory consumes gigabytes without improving runtime
isolation. Cache hits do not replace source/output verification or correctness tests.
Do not change global cache configuration as a side effect of a campaign.

The pair retained at the initial P01 cleanup is a reusable seed. It has since been
advanced in place through P07 diagnostics and P04/P05 comparisons, with each older
native replay explicitly retired before replacement. Use retained refs and campaign
receipts to identify its current source and outputs; directory names do not identify
a qualified experiment. Source, probe, build and environment changes require new
applicability checks. Preserve shared correctness behavior when varying an algorithm.

## Isolated benchmark launch

Launch both the Python namespace wrapper and its inner checker with `-I -S -B`
when using `--isolated-python`. Pin the actual controller interpreter and verify
required pidfd/subreaper capabilities before campaign artifacts. Keep the existing
bounded cleanup limited to descendants owned by that standalone controller.

A protocol may capture locked Pixi activation once and launch its exact native
Python directly for each fresh process. Freeze the capture operation, selected
environment mapping, interpreter/module/native closure, and resolved tools; qualify
the final command before sampling. Construct worker environments from that mapping.
Keep environment values and XAUTHORITY locators private and exclude credentials;
never collect authentication cookies.

Give each arm independent HOME, configuration, FreeCAD data, temp, Mesa and XDG
state. Inventory every writable tree before and after a campaign, and reject
symlinks in controlled fresh cache trees. A Mesa/XDG-only inventory does not cover
preferences loaded from HOME/configuration or files retained in FreeCAD data/temp.
Avoid repeated environment validation/activation inside an observation loop: perform
full frozen-input checks before and after collection, and keep per-worker setup
outside the declared timing endpoint.

## Preserve controlling source and failed campaigns

Before freezing a campaign, copy every mutable controlling script, imported local
helper and policy/rubric document into a byte-exact source capsule. Record a manifest
from each original path to its content hash and capsule file. Verify both copies,
then bind the capsule and manifest in the freeze. A hash of a live root-worktree
file alone cannot recover the reviewed implementation after that worktree advances.
Keep native source refs and actual binary/build identities separately.

If review discovers a fidelity or provenance gap during collection, stop the owned
standalone controller through its pinned process handle so its existing finally
path cleans only its own descendants. Retain the failed status, all complete and
partial observations, raw output, abort reason and source capsule. When possible,
audit the unchanged inputs/runtime before changing the verifier. Record the exact
hashes and sizes of this disposition evidence and bind that receipt in any corrected
campaign; never backfill missing verification into old observations.

A verifier repair needs fresh qualification, independent review, distinct artifact/
cache destinations and a new frozen complete schedule. Mark the earlier attempt
ineligible and keep it separate; do not pool or replace individual observations.
This recovery is different from repeating qualified measurements to clear a numeric
review trigger. The governing acceptance policy decides whether such a trigger
needs further work.

## Recorded retirement

The 2026-11-08 Spec 003 retirement removed two redundant P01 compiler caches and
four completed T020 build-output directories, reclaiming 16.54 GiB. It retained all
six source checkouts/refs, the P01 build pair, campaign evidence and `build/debug`.
The exact targets and preservation checks are in
[the storage retirement record](../specs/003-freecad-performance-optimization/storage-retirement-20261108.json).
The T020 native outputs are retired; their historical measurements remain recorded.
