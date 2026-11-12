---
title: Assembly motion suspension expert review
doc_type: spec
artifact_type: history
status: archived
owner: local-developer
last_reviewed: 2026-11-12
---

# Private MoE Review

Reviewers are independent AI roles, not actual FreeCAD maintainers or upstream approval.
Native FreeCAD App/Assembly expert, Python/workbench expert, and test/lifecycle expert
inspect direct source and draft artifacts. Review messages and dispositions follow.

Initial findings incorporated into draft: guard before native solve preparation; document
identity must survive closure safely; Python guards use joint.Document before containment;
cover angle/rigid/undo writers, AssemblyLink synchronization/conversion and simulation;
distinguish allowed link geometry from writable components; no purgeTouched recovery claim.

## Draft review disposition (2026-11-12)

- Native N1 / Python P1 / Validation V1 BLOCKER: D001 resolved with pre-change populated
  Rigid rejection including same-value assignment, empty/restore initialization exception,
  unchanged mode/group/grounding/poses test, and existing placement-sync suppression.
- Native N2 / Validation V3 BLOCKER: finite supported admission matrix added; inspect raw
  endpoints/ground/rigid/motion targets and detect cycles; reject unsupported/incomplete
  records before mutation, avoid getRigidGroups repair helpers.
- Validation V2 BLOCKER: guard entry distinguished from solve preparation; native nonidentity
  LinkGroup preparation oracle plus private-process GDB instrumentation planned with controls.
- Native N3 REQUEST: -7 suspended / -8 invalid admission; void helper no-op documented.
- Native N4 REQUEST: no new purgeTouched for recovery; existing solve behavior retained.
- Native N5 / Python P2 REQUEST: private per-operation write allowance, automatic callbacks
  remain blocked, grounded status reconciled after admission, final status refresh non-solving.
- Validation V4/V5 REQUEST: test files included in incremental slices; doc isolation, single-use
  context/GC and rejected-graph touched/status/undo checks added.

All draft blockers addressed in artifacts before implementation. These are design dispositions;
implementation and test evidence remain pending. Three expert roles independently reviewed all
six artifacts; actual FreeCAD maintainer approval is not implied.


## Implementation review findings and disposition

- I001 BLOCKER: old PyCXX instance methods did not implement Python with lookup. Reproduced
  TypeError in built private process; native tp_methods/readyType fix passes simple context probe.
- I002 BLOCKER: raw AssemblyLink chain could recurse before admission cycle checks. Iterative
  chain validation added before recursive helpers.
- I003 BLOCKER: missing Motion.Joint could be ignored. Structural SimulationGroup drive checks
  and supported Angular/Linear validation added.
- I004 BLOCKER: suspended ground retarget/delete left stale locks. Native weak+ID deferred
  release plus Python deletion callback added; invalid admission retains intents. Test pending.
- I005 REQUEST: Python frame binding returns None; native return -7 distinguished in docs.
- Restore fixture: plain App::Part control shows App::Link representation normalization outside
  Assembly. Preserve exact copy poses and effective world poses on restore; do not claim local
  matrix invariance across core serialized normalization. Native preparation instrumentation
  still independently required.

- I006 BLOCKER (runtime/GDB): Document::restore emits signalDeleteDocument before replacing
  contents, so using that signal as lifetime termination dropped suspension. GDB observed one
  native solve and preparation during restore. Registry now preserves live depth through
  Delete/New content notifications; actual DeletedDocument/map removal invalidates closed
  document states. Recreated-object intents are cleared before ID reuse. Rerun pending.

## Final local review result

Four independent expert roles participated: FreeCAD native App/Assembly, FreeCAD Python/workbench,
validation/lifecycle, and an independent native API/lifetime reviewer. All draft/implementation
blockers I001–I006 are resolved in production commit f0dd422402 and regression commit a73012a8cd.
Final lifetime re-review checked fake Delete/New vs actual DeletedDocument order and no stale-key
dereference. Runtime evidence supersedes earlier source-only readiness: ten native and 39 Python
core cases pass; GDB confirms no hidden preparation in exercised guard/restore phases with positive
controls. Broad suite remains non-green only for two unchanged mock failures, E001/B015.
This is private AI expert review, not actual FreeCAD maintainer approval. Human review/testing,
issue/API agreement, personally authored communication and AI disclosure are still required for PR prep.
