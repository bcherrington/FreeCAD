---
title: Camera-Adaptive Tessellation Design
doc_type: spec
status: draft
owner: bcherrington
last_reviewed: 2026-08-14
artifact_type: design
---

# Design

## Overview

Introduce a display-only adaptive mesh controller between viewer camera changes
and Part view-provider mesh replacement. The controller selects a small quality
tier from screen-space error, debounces camera motion, applies hysteresis, and
accepts only complete meshes that satisfy configured budgets.

## Template Authority Decision

No repository-specific lifecycle template was found. This package follows the
installed Spec Lifecycle Manager fallback structure and the conventions used by
the reconciled Spec 009 package.

## High-Level Design

```text
camera/object event -> eligibility + screen-space tier -> cache lookup
  -> bounded/cancellable mesh request -> validate budgets -> atomic publish
  -> last-known-good/native fallback on miss, cancellation, or failure
```

### D1 Policy and hysteresis

Use projected object bounds, projection type, device-pixel ratio, and a small
fixed tier set. Enter and leave thresholds differ. Camera motion schedules work
only after a quiet interval; stable states are idempotent.

### D2 Cache and invalidation

Key entries by object identity, geometry revision, relevant display state, and
tier. Account memory explicitly. Geometry changes invalidate that object;
camera changes normally choose another tier without discarding reusable meshes.

### D3 Bounded generation

Estimate cost before starting and provide cancellation/early-abort checkpoints
during generation. A result is publishable only after measured triangle,
memory, and elapsed-time checks pass. Background work is permitted only after
thread ownership of OCCT, Coin nodes, and GUI publication is proven safe.

### D4 Atomic publication and fallback

Build away from the live scene graph, then publish atomically on the GUI thread.
Keep the last-known-good mesh visible while work is pending. Failure, cancellation,
or budget breach retains that mesh or the native mesh and records the reason.

### D5 Observability and gating

Expose opt-in counters for policy decisions, cache hits/misses, cancellations,
generation time, triangles, memory, and fallback. Production defaults remain
unchanged until deterministic and representative-model gates pass.

## Low-Level Design

- Reconcile whether existing Part meshing can expose preflight estimates and
  cooperative cancellation without duplicating OCCT tessellation logic.
- Define the immutable handoff from generated mesh data to GUI-owned Coin nodes.
- Decide cache owner and eviction policy only after actual memory measurements.
- Prefer extending a generic view-provider refresh contract over repeated broad
  property-map walks if the current update interface proves too expensive.

## Operational Considerations

- Start disabled behind an explicit experimental switch.
- Never write adaptive property changes into the document.
- Capture Debug and Release behavior separately; Debug timing is diagnostic.
- Test X11 first, then route Wayland, Windows, and macOS gaps explicitly.

## Risks

- OCCT tessellation may not offer sufficiently granular cancellation.
- Concurrent geometry mutation can invalidate work in flight.
- Cache memory can exceed the cost saved by remeshing.
- Screen-space bounds may underrepresent thin/high-curvature features.
- Maintainer consensus is required before adding persistent preferences or a
  background meshing service.

## Open Questions

- Can current OCCT tessellation expose cooperative cancellation cheaply enough?
- Which layer should own cache limits and eviction policy?
- Is a generic targeted view-provider refresh contract needed before integration?

## Downstream Review Note (2026-08-14)

This design file was reviewed with `tasks.md` and `requirements.md` for
downstream consistency under the current canonical context. No implementation
scope changes were introduced.
