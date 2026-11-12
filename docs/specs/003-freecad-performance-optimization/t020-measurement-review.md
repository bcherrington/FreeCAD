---
title: T020 measurement qualification and isolation review
doc_type: spec
artifact_type: review
status: in-progress
owner: local-developer
last_reviewed: 2026-11-07
---

# T020 Measurement Qualification And Isolation Review

This is a pre-sample execution clarification under D001, recorded by Codex for
local-developer under the instruction to complete T020. It is delegated technical
selection with an independent agent review, not human or maintainer approval.
At this historical pre-sample checkpoint, no acceptance samples had been collected. The numeric plan, thresholds, fixture
identities, sampling schedule and historical receipts remain unchanged.

The later first collection is now recorded in
[performance-results.md](performance-results.md): six complete cells, an aborted
hidden primary and two inconclusive GUI timing outcomes. T020 remains incomplete;
D004 tracks recovery. The exact pre-sample bytes bound by freeze 1 are retained in
[t020-evidence/freeze-1-pre-sample-measurement-review.md](t020-evidence/freeze-1-pre-sample-measurement-review.md).
The qualification-in-progress section below is historical, not current status.

## P06 Reviewed Exclusion

**Disposition: independently non-identifiable; subsumed by the generalized
P07/T006 lifecycle. No separate P06 performance improvement is claimed.**

The historical P06 change (`e093a1f66b3f921961c7a936924f58b88637bef8` to
`cdf2a84f4ba36518c3cd5de977a20ef6c98721a4`) skips an ImageFile load when restoring
and the payload does not yet exist. Current `ViewProviderImagePlane::updateData`
defers every nonempty image/size update throughout GUI restore and App import.
Keeping that later guard makes the earlier P06 condition unreachable in both
arms. A positive failed-load baseline would require bypassing the generalized
guard, violating Requirement 5 AC1 and the safety-preserving construction rule.

Read-only reviewer `t020_isolation_review` inspected the current source, both
historical deltas, requirements, protocol, plan and runner. The reviewer confirmed
this conflict and recommended the explicit exclusion route allowed by Requirement
3 AC3 and the protocol's Revision And Build Contract. The task owner adopts that
route before sampling. Do not synthesize counters, disable the guard, execute an
unsafe historical baseline or present a combined speedup as P06 evidence.

T006 zero-premature-load/lifecycle correctness remains required. T020 also records
actual failed pre-restore loads in P07 executions; zero is a correctness invariant,
not evidence of an independently measured P06 speedup. T004 and T010 must retain
this exclusion in their optimization-disposition inventory.

## P07 Arm Construction And Correctness Boundary

The safe baseline eagerly loads every pending texture at the payload-ready
`finishRestoring` barrier; the change retains the visibility condition. The eager baseline omits that
visibility guard; the lazy change retains it. This condition is the only
performance delta. Both arms retain restore/import guards,
`ImageSizePolicy::Preserve`, full resolution, error handling, calibration and
first-show logic. All-visible execution is intentionally equivalent.

Freeze arm-specific pending/materialized expectations before samples: an eager
baseline has materialized hidden textures, while the lazy arm keeps them pending.
The existing test's hidden-texture-absence assertion describes the lazy arm and
cannot be imposed on an eager baseline. Shared pixel, resolution, placement,
calibration, reference identity, import, error and save/reopen checks still apply
to both arms. The selected image must converge after first show.

The shared normative state digest covers model/object/shape/reference identities,
placements, calibrated dimensions and original image identities. It excludes
hidden-cache presence. Pending/materialized counts are separate explicit boundary
observations. Do not materialize unselected hidden images just to make cache state
equal; that would change the resource workload.

For non-shape references, use document-object resolution (`getSubObject(name, 1)`)
instead of assuming default `getSubObject()` returns a TopoShape. Validate actual
geometric elements separately. Virtual sketch axes use their canonical axis API.
The review confirmed that a default `None` for an App::Plane/Origin reference does
not by itself indicate an invalid fixture. Any genuine unresolved geometric
reference remains a qualification failure.

## Probe Qualification In Progress

- The checksum-pinned private fixture is available. A temporary copy opens in the
  development build and its required Gridfinity proxy restores. The installed
  Gridfinity package declares 0.12.4 and imports successfully. Original bytes are
  unchanged. Private locators, model contents and screenshots are not published.
- The launcher uses fresh temporary preferences/data/fixture copies, an isolated
  copy of the pinned Gridfinity addon and the locked development environment.
- A Debug public-document preflight observed native X11, Intel Mesa, a physical
  1280×720 viewport, DPR 1 and eight effective samples in the native framebuffer.
  This is capability/probe evidence, not optimized timing acceptance.
- Frame completion observes a swap after the request, makes that view's context
  current and waits for GL completion. It resolves/reads that same native
  framebuffer without `grabFramebuffer`, which would render another frame.
  Pixel/reference verification happens outside the primitive timer. No monitor
  scanout or GPU-residency claim is made.
- Opt-in developer instrumentation compiles. Focused tests pass for shared decoded
  storage, direct/fallback conversion overlap, Coin copies, cleanup and detection
  of an unobserved backing-store change. Instrumentation is disabled by default.
- The statistical assessor passes fourteen synthetic tests covering percentile
  interpolation, bootstrap/saving floors, maximum resource comparison, tail
  non-regression, variability, zero-baseline/count behavior and invalid metrics.
  Synthetic observations are never application performance evidence.
- Remaining qualification includes native private-image reference checks, exact
  resource scope/overlap, shared correctness on matched optimized arms, isolated
  build provenance, context/worker/fixture/receipt hashes and the execution freeze.
  No result is acceptance-eligible until these gates pass.

## Logical Resource Scope

The existing `decoded_image_bytes` and `coin_image_bytes` keys measure maximum
simultaneously live **image-plane-workload-owned** decoded QImage buffers and
SoSFImage field payloads, respectively. Count shared backing pointers once and
include source/fallback conversion and temporary/persistent Coin copy overlap.
These are logical payload bytes; allocator capacity, codec scratch, unrelated Qt
icons and non-image-plane Coin textures are outside these two counters.

`peak_rss_bytes` remains the Linux process-wide high-water measurement, including
startup, fixed probe storage and driver/allocator effects. It is not derived from
the logical counters. The fixed CPU framebuffer readback buffer is allocated,
touched and reused before the open timer; correctness decoding and reference
replacement occur after the recorded resource window. Framebuffer resolve/driver
work during the window remains part of the observed process footprint.

The TestVRMLTextures control has no image-plane workload: its two logical counters
are expected zero and do not measure its unrelated VRML textures. Process-wide RSS
and completed-frame correctness still apply. Neither counter establishes GPU
residency. Reviewer `t020_isolation_review` accepted this bounded resource meaning
provided it is frozen explicitly before samples; this section and its hash must
be included in the execution qualification receipt. No numeric resource budget
is changed.

## Reviewed Incremental Arm Construction

Reviewer `t020_isolation_review` conditionally accepted deriving the isolated
baseline build from the freshly qualified common outputs. Both arms must use the
same hashed namespace wrapper, canonical CMake paths, preset, locked environment
and explicit shared Mesa/XDG cache policy. The physical worktrees/builds remain
separate. Copied regular outputs must have independent inodes; all unchanged source
and generated/dependency inputs are checked by content hash before synchronizing
source mtimes. Ninja logs and command hashes are not edited.

Force the provider translation unit through the real compiler and relink Gui in
both arms with `CCACHE_DISABLE=1`, retaining verbose commands and object/library
hashes. Reused common outputs must be enumerated truthfully. Verify exact commits,
source cleanliness, the one-condition source delta, configured paths, actual loaded
arm libraries, matching model/frame state and full arm-specific correctness.
Remount builds read-only for qualification/collection and rehash before/after.
This is a copied-common incremental baseline, not an independently clean rebuild.

The original controller owns GUI cells. The separate command collector uses the
same fixed 24-pair schedule/bounds for descriptive command-only observations; it
cannot replace a GUI cell or establish an independent optimization speedup.

## Execution Boundaries

The initial qualification checkpoint is
`8a57eb35fed417b0d1485fa4f860e9814b8c642c`, based on
`42192242fd77542b3cb9df68432e0b0effae1e71`; this is an internal detached source
checkpoint, not the final qualified common base or an execution freeze. Subsequent
probe corrections must be committed into the final common base and rebuilt as
needed. The user's working branch is not moved by these internal checkpoints.

Pixi environments are not relocatable. A worktree-local `.pixi` symlink was
rejected by Pixi; the environment was not recreated. Run the locked root Pixi
environment with explicit worktree source/build overrides:

```sh
pixi run --locked cmake --preset conda-linux-rel-with-deb-info \
  -S build/spec003-t020-worktrees/common \
  -B build/spec003-t020-worktrees/common/build/relWithDebInfo \
  -DBUILD_IMAGE_PERFORMANCE_PROBE=ON
```

Record every actual configure/build command and outcome in the eventual build
receipt. Existing Debug numbers, unqualified binaries and incomplete collections
remain ineligible. T020 stays in progress.

## Native VRML Readiness Qualification

Repeated pre-sample qualification exposed a completed but untextured VRML frame.
Coin schedules VRML image decoding through its idle queue after first traversal.
The endpoint therefore observes nonempty-URL textures in the rendered scene,
issues the initial render to trigger lazy loading, and attaches priority-100
node sensors to pending textures. A load-completion touch must observe
`SbImage::hasData()` before a subsequent natural Coin render, native swap and
GL completion. The root schedules at priority 1 and redraw runs at 10000.
The probe records the render generation at readiness and accepts only a later
render. It does not force a post-readiness redraw or bypass Coin's notification
latency. Event processing and that render remain inside the primitive timer. No lazy-loading setting is changed and no
pretiming texture decode is introduced. The texture count is frozen per context;
the existing exact reference-frame check still rejects an incorrect result.
The endpoint remains bounded to ten seconds. These failed qualification runs
are preserved as diagnostics and are not acceptance samples.

Optimized runtime qualification also required the normal Part, PartDesign,
Sketcher and Mesh script copy targets, GUI resources/stylesheets and Python
embedding modules. The first selected native-library targets alone were an
incomplete runtime. Both arms contain the same completed runtime inputs.
