---
title: Sketcher reattachment local delivery review
doc_type: history
status: active
owner: local-developer
last_reviewed: 2026-11-08
---

# FreeCAD Maintainer Pre-Review

**Verdict:** READY FOR HUMAN PR PREP for the local implementation.
**Risk:** Medium (public API, shared projection rebuilding and solver status).
**Base reviewed:** Sketcher subtree at `454fe91954` and subsequent task-owned
changes; independent GUI/spec003 work excluded. The solver correction is its own
commit, `42144bc1e9`.
**Scope:** Native scalar reattachment, generated Python binding, lifecycle tests,
authored projection modes, solver failure propagation and durable reference.

This is a private, simulated expert review by native Sketcher, API/maintainer-style,
transaction/observer and test specialists. It is not actual FreeCAD maintainer
consultation or upstream approval. The human owner remains responsible for review,
testing, public communication and current AI_POLICY.md obligations.

## Findings and Dispositions

| Finding | Disposition and evidence |
| --- | --- |
| Sibling non-edge references incorrectly fail duplicate admission | Fixed by generic sibling resolution and edge-only alias comparison; public vertex-sibling case passes |
| Source/plane restrictions omitted from Python help | Part::Feature and parallel circular-plane restrictions now explicit in the generated .pyi help |
| Arc tests checked endpoints without consuming those roles | Added an internal line constrained to arc start/end; recompute/replay/reopen assertions pass |
| Construction mode/layer lost during ordinary projection rebuild | Same-ID replacement deep-copies only the old SketchGeometryExtension; fresh external Ref remains intact; native persistence test passes |
| Cross-object dependency restoration insufficiently tested | Native injected errors cover both retained-old-source variants; public abort/undo/redo/reopen checks validate backlinks/outgoing links |
| Centre motion reports solver success without moving under LM | Isolated disconnected-subsystem regression fails before the status fix; first non-success now reaches ordinary fallback; native strong motion checks and all public solver variants pass |
| Exact Tangent Orientation needs native evidence | Native signed-line test compares Distance and Tangent Orientation across recompute, undo/redo and restoration; public Tangent side independently survives the lifecycle |

All four specialists found no remaining blocker within their inspected scopes
after corrections. Their source reviews did not independently execute every
validation command. Runtime claims below come from the primary agent's retained
receipts, not from a claim of independent reruns.

## Validation Performed

- `pixi run build -j 2`: passes on Linux using the existing debug environment.
- All native Sketcher cases: 163 pass, including thirteen feature cases and the
  new disconnected-subsystem regression with explicit fallback recovery.
- Public feature suite: sixteen cases pass under each solver selection 0/1/2,
  forty-eight executions without failures/errors/skips.
- Full `TestSketcherApp`: 113 run, zero failures/errors, one pre-existing secant
  support skip. The separate existing solver-class run has 27 cases/one same skip.
- Final owned-profile full CTest: 2,434 registered: 2,422 pass, two fail, two runtime
  skip and eight disabled. 2,426 enabled processes were invoked; skips are not passes. The project is not reported fully green.
- Scoped hooks, region formatting and CRLF-aware diff checks cover owned files.
  Exact commands, case manifests and artifact hashes are retained with spec 004.

## External and Inherited Gates

B012 owns the isolated Measurement handler-null failure. B013 owns the Part loft
selector fixture failure, which creates Part line profiles without calling
Sketcher. Both reproduce in the initial and final repository runs; their source
modules are outside this feature's changes. Their root causes are not claimed
fixed. See [owned follow-ups](../backlog/reattachment-delivery-follow-ups.md).

B014 owns clean-base/separate-worktree PR preparation, Windows/macOS CI, issue/API
alignment and personal human review. No existing default, migration or dependency
was introduced. Existing grouped/vector API behavior remains intact, with the two
explicitly reviewed shared correctness fixes above. The human owner should keep
the solver correction independently reviewable and assess release communication
under the target upstream's current process. No release announcement is inferred
from local documentation promotion.
