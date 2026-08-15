---
title: Active specification index
doc_type: reference
status: active
owner: bcherrington
last_reviewed: 2026-08-14
---

# Active Specification Index

Active implementation packages use `docs/specs/[###-slug]/` with a unique,
zero-padded three-digit sequence. The next package number is `016`.
Historical records retain legacy aliases and commit evidence when their
canonical spec ID is renumbered.

Repository-specific spec-package templates are intentionally absent. New and
migrated packages use the Spec Lifecycle Manager's bundled fallback templates
and must contain
`requirements.md`, `design.md`, `tasks.md`, `traceability.md`, and
`verification.md` once they reach verification stage. Optional research,
quickstart, decision, impact, and canonical-context artifacts are added only
when they reduce ambiguity.

## Active Packages

| ID | Package | Previous ID | State |
|----|---------|-------------|-------|
| 002 | [Upstream PR readiness](002-upstream-pr-readiness/requirements.md) | `upstream-pr-readiness` | revised; branch planning-ready, not PR-ready |
| 004 | [Performance investigation](004-performance-investigation/requirements.md) | `20260524-performance-investigation` | draft |
| 010 | [Camera-adaptive tessellation](010-adaptive-tessellation/requirements.md) | none | draft; production architecture planning-ready |
| 014 | [CAD and mesh file thumbnailing](014-cad-file-thumbnailing/requirements.md) | `cad-file-thumbnailing` | draft; requirements and design planning-ready |

## Migration Rules

- Preserve prior IDs in each package's migration record.
- Update current cross-package links atomically when a package is renamed.
- Do not infer implementation or verification from a migrated checkbox.
- Completed tasks require concrete evidence; pending evidence remains pending.
- Run lifecycle package lint and link/static checks after structural changes.
- Promote accepted behavior and decisions to durable documentation before
  closing or archiving a package.

## Historical Specifications

See [the archive index](../history/spec-archive-index.md) and
[closure log](../history/spec-closure-log.md). The compact-UI package is now
`008-compact-ui-idle-cpu-loop`; its original
`20260607-compact-ui-idle-cpu-loop` ID remains recorded as a legacy alias.
Specs `001`, `003`, `005`, `006`, `007`, `008`, `009`, `012`, `013`, and `015` are
closed after their lasting findings, startup behavior, async branch guidance,
GPU diagnostics, viewport experiments, depth-aware contrast decisions,
compact shortcut-hosting behavior, and BRep edge-AA decisions were promoted to
durable documents.

## Transferred Specifications

Spec 011, Display preset macros, moved on 2026-08-14 to the implementation
repository at
`/home/bcherrington/Projects/CLion/freecad-appearance-macros/docs/specs/011-display-preset-macros/`.
The stable ID remains reserved, but this FreeCAD documentation repository no
longer owns or lists the package as active.
