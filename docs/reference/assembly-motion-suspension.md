---
title: Assembly automatic motion suspension
doc_type: reference
artifact_type: reference
status: code-derived
owner: local-developer
last_reviewed: 2026-11-12
---

# Assembly Automatic Motion Suspension

This reference describes the implemented API on `feature/assembly-motion-suspension`
(commits `f0dd422402`, `a73012a8cd`). It is not yet a released FreeCAD API. Implementation and qualification status is recorded in
[Spec006 verification](../specs/006-assembly-motion-suspension/verification.md).

## Use

```python
import AssemblyApp

with AssemblyApp.suspendAutomaticMotion(document):
    assembly = document.addObject("Assembly::AssemblyObject", "Assembly")
    joints = assembly.newObject("Assembly::JointGroup", "Joints")
    # Create local components; copy/remap joint, grounding and drive references.
    assert AssemblyApp.isAutomaticMotionSuspended(document)
    result = assembly.solveValidated()
    if result != 0:
        raise RuntimeError(f"Assembly solve failed: {result}")
```

Enter the context before assembly creation or copying. It affects automatic Assembly motion
in that live document, including callbacks on joints not yet in an assembly. Native code uses
`Assembly::AutomaticMotionSuspension` from `MotionSuspension.h`.

Contexts nest, are single-use and release after exceptions or garbage collection. Closing a
document invalidates its suspension state without affecting a new document with the same name.
Leaving a context never solves, replays callbacks, changes preferences or rolls back edits.
Use the normal application thread; this facility is not a thread synchronization primitive.

## Motion and metadata

Ordinary native solve/simulation/frame calls return `-7` while suspended. Python solve and
generateSimulation expose that result; the existing Python updateForFrame binding returns
None and performs no motion. Void placement helpers
perform no motion. Joint references, connector metadata and property migrations can still be
constructed. Native status refresh preserves cached diagnostics while suspended; those values
may be stale. No completeness or freshness should be inferred from them.

Populated AssemblyLink `Rigid` assignments reject before changing mode, placements or grounding.
This includes same-value assignments because the pre-change notification lacks the incoming
value. Empty-link initialization and serialized restoration remain supported. Existing rigid
link component poses are not synchronized while suspended; new local instances may receive
initial copied poses. Serialized App::Link array restore can redistribute its transform
into element coordinates while preserving effective world poses, as it does outside Assembly. This distinction permits structural copying without automatic positioning.

## Explicit solving and references

`solveValidated(enableUndo=False)` checks the supported raw assembly graph before solve
preparation. It rejects invalid/escaped references with `-8`; ordinary solver result codes are
retained after admission. A local App::Link may reference external source geometry: that source
is read input, not an admitted target for placement writes. Grounding, rigid members and native
Motion.Joint references must resolve within the supported local assembly closure.

Ground retarget/deletion defers old lock cleanup until successful admission; failed validation
leaves lock states unchanged, and unrelated manual locks are preserved.

Automatic callbacks remain suspended during the explicit solve. The operation does not clear
document suspension or grant permission to other automatic solve, frame or undo entrypoints.
The caller still owns construction correctness and must inspect the return code.

## Limits

This API controls Assembly motion, not arbitrary Python or third-party observers. It is not a
sandbox, transaction rollback, arbitrary-document restore interceptor or promise to reproduce
every document touched flag after edits. It adds no persistent flag or preference. MCP graph
admission, receipts and exact bridge recovery remain responsibilities of that integration.

## Change rules

Native source and public binding documentation are authoritative for actual behavior. Update
this reference with API/result or supported-graph changes, and rerun the linked regression
suite. Do not infer availability from an installed version without identifying the built
AssemblyApp binary and source revision.

## Qualified local build

Linux Clang 21.1.8 / Python 3.13.18 / Qt 6.11.3 / OCCT 8.0.1, FreeCAD 27.1.0dev Dev Build.
AssemblyApp SHA256: `1e30bf20da8edecea3f187b6d8b05b840dc99dd15e94653c9bb103286befb416`.
Ten native cases and 39 Python core cases pass. GDB records zero native solve preparation in
exercised suspension/restore phases with working positive controls. Broader workbench: 59/61,
two pre-existing view mocks need Label (B015). See Spec006 evidence for exact commands/limits.
Detect the callable module context/introspection functions and assembly.solveValidated; version
text alone is insufficient. Origin/root datums are admitted as grounds, not explicit endpoint
roots. Supported native MotionTypes are Angular/Linear with compatible native joint kinds;
formula syntax/execution remains the native simulation evaluator's responsibility.
