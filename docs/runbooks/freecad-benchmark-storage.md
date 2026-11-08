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

The retained P01 pair compares scalar and buffered restore. It is a seed for later
work, not an already-qualified P07 comparison: both future P07 arms must retain the
same buffered restore and safety fixes, differing only in the reviewed visibility
condition. Source, probe and environment changes require new applicability checks.

## Recorded retirement

The 2026-11-08 Spec 003 retirement removed two redundant P01 compiler caches and
four completed T020 build-output directories, reclaiming 16.54 GiB. It retained all
six source checkouts/refs, the P01 build pair, campaign evidence and `build/debug`.
The exact targets and preservation checks are in
[the storage retirement record](../specs/003-freecad-performance-optimization/storage-retirement-20261108.json).
The T020 native outputs are retired; their historical measurements remain recorded.
