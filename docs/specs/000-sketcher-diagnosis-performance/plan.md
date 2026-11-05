---
title: Sketcher diagnosis performance implementation plan
doc_type: spec
artifact_type: plan
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Implementation Plan

## Summary

Capture a deterministic baseline, add optional phase diagnostics, remove duplicate
setup locally, validate behaviour and latency, then select further work from data.

## Technical Context

- **Language/Version**: repository C++23 and existing Python test support.
- **Dependencies**: existing Eigen, Qt and OpenCASCADE; no additions.
- **Storage**: no persistence change; private benchmark receipts outside source.
- **Testing**: native Sketcher/App coverage plus disposable GUI model checks.
- **Platforms**: Linux first for reproduction; preserve Windows/macOS compilation.
- **Performance goals**: requirements AC-005/AC-006; no claimed achieved speedup.
- **Scope**: Sketcher App, planegcs, affected native tests and diagnostic docs.

## Governance Check

- [x] Responsibility boundaries: D-001/D-002 keep reporting and solver logic local.
- [x] Reuse policy: existing guards/time/reporting before introducing new helpers.
- [x] Test strategy: each task maps to FR/AC and verification cases.
- [x] User impact: no changed modelling interaction or public error semantics.
- [x] Proposed budgets and comparable measurement method defined in AC-005/AC-006.

These checks describe the plan, not evidence that implementation has passed.

## Project Structure

The repository templates in `docs/templates/spec-package/` govern this package.
The local spec/design/plan/tasks/research/quickstart shape is retained. To support
the lifecycle reader, normative FR/AC text has a single home in `requirements.md`;
`spec.md` is the scenario/overview entry point. `traceability.md` and
`verification.md` provide explicit routing and evidence. This is a new-package
format choice, not migration of any existing package.
The creation tool proposed ID 000 because existing packages have no numeric IDs;
its fingerprint was revalidated before atomic directory creation. No historical
number is claimed or legacy package renamed. Purpose comes from the user's
explicit request and the preceding discussion.

Expected source boundaries:

- `src/Mod/Sketcher/App/SketchObject.cpp`, `SketchObjectConstraints.cpp`.
- `src/Mod/Sketcher/App/Sketch.cpp`, `Sketch.h`, and `planegcs/GCS.cpp`/`GCS.h`
  only where the selected phase-reporting implementation requires them.
- `tests/src/Mod/Sketcher/App/` and existing CMake wiring where necessary.
- Durable diagnostics reference/runbook during promotion.

## Phases

1. T001: reconcile source, fixtures, call counts and existing settings.
2. T002/T003: finalize trace contract and implement/test optional timings.
3. T004: implement/test local duplicate-diagnosis removal.
4. T005: verify correctness and benchmark matched configurations.
5. T006: rank and route remaining optimization candidates.
6. T007: promote verified documentation and reconcile closure evidence.

## Dependencies

The crash fix is already on this branch. Existing operator models remain
external private fixtures. Implementation readiness requires T001/T002 findings;
profiling must not depend on an unavailable MCP server for native attribution.
Only one implementation task should be in progress at a time.

## Risks

- Lost notifications or diagnostic validation: retain events and test observers.
- Undo/external-reference changes: compare native results, not process survival.
- Logging distorts timings: collect tracing-on attribution and tracing-off latency.
- Concurrent QR reporting races: collect safely, emit through a safe owner.
- Debug/release or preference mismatch: require complete baseline receipts.
- Overfitting one model: use synthetic, master, plate and diagnostic-error cases.

## Validation Strategy

Use `pixi run build -j 2` for normal build verification. Select native targets
and CTest filters by inspecting current registration in T001, then run focused
Sketcher/App tests before broader affected suites. Validate GUI-only behaviour
on separate disposable processes. Run applicable pre-commit hooks before commit.
Do not rebuild or replace the user's snapshot as part of spec creation.

Acceptance and evidence routing are specified in [verification.md](verification.md).
