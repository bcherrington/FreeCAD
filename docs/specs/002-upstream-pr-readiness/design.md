---
title: Upstream PR readiness for FreeCAD topic branches design
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: design
---

# Design

## Canonical-Context Review

Reviewed 2026-08-14 against the explicit canonical context. Existing design
boundaries remain valid; live contribution policy and current branch evidence
override package summaries when they conflict.

## Overview

This package now operates as a branch-readiness registry rather than a
single massive checklist. It records one fetched `upstream/main` baseline,
one shared policy baseline, one canonical candidate inventory, one reusable
readiness template, and one readiness record per active candidate branch.

## Package Identity

- Current package: `docs/specs/002-upstream-pr-readiness/`
- Legacy package ID: `upstream-pr-readiness`
- Current fetched baseline: `upstream/main` at
  `7937058dededc800dd0f3197185daf75d146c9d6`
- Active candidate count: eight canonical branches
- Alias-only or obsolete local names are routed from the matrix in
  [research.md](research.md); they are not treated as active candidates.

## Artifact Layout

| Artifact | Role |
|----------|------|
| [requirements.md](requirements.md) | Package scope, gates, and readiness requirements |
| [research.md](research.md) | Current upstream baseline, policy baseline, candidate matrix, alias routing, and external references |
| [tasks.md](tasks.md) | Executable next actions for the package and for each branch record |
| [verification.md](verification.md) | Package-level lint, patch-integrity evidence, and branch validation gates |
| [traceability.md](traceability.md) | Requirement-to-task-to-record mapping |
| [readiness-record-template.md](readiness-record-template.md) | Reusable schema for candidate branches |
| `readiness-records/*.md` | Branch-specific readiness records for active canonical candidates |

## Durable Baseline And Impact

### Durable Baseline

- `CONTRIBUTING.md`
- `.github/pull_request_template.md`
- `AI_POLICY.md`
- Official Developers Handbook pages used for PR preparation and review
- Official GitHub PR pages for current policy-discussion watch items

### Durable Impact

- Package-local readiness records are the temporary system of record while these
  branches remain local.
- If recurring rules emerge from this work, they should be promoted before
  package closure.
- No source behavior changes are part of this slice.

## Staged Readiness

| Gate | Status | Conditions for passing | Current state |
|------|--------|------------------------|---------------|
| `ready_to_implement` | Ready | Baseline, policy, matrix, template, and per-branch records are current. | Passed by this revision. |
| `ready_to_validate` | Not ready | At least one candidate record has issue/PR overlap resolution, cleanup plan, and selected validation plan. | No branch has reached that state yet. |
| `ready_to_close` | Not ready | All branches are routed to ready, blocked, deferred, or obsolete with promoted durable knowledge. | Incomplete. |
| `ready_to_archive` | Not ready | Closure metadata and final package truth are complete. | Incomplete. |

## High-Level Design

### Workflow Architecture

1. **Fetch and pin a review-cycle baseline.**
2. **Summarize the currently binding contribution rules.**
3. **Reconcile canonical candidate refs and alias drift.**
4. **Create or update one readiness record per active candidate.**
5. **Route obsolete, composite, or alias-only branches out of the active path.**
6. **Use the readiness records to perform issue research, overlap review,
   validation planning, cleanup planning, and sign-off one branch at a time.**

### Inputs

- current local topic branches
- current `upstream/main`
- current FreeCAD contribution and review policy documents
- current GitHub state of relevant policy-discussion PRs

### Outputs

- one candidate matrix with explicit dispositions
- one readiness template
- one readiness record per canonical candidate branch
- smaller executable tasks tied to those records

## Canonical Disposition Model

Each local branch referenced by this package must be in exactly one of these
states:

- `candidate-clean`: active candidate; a rebase onto current `upstream/main`
  may be enough.
- `candidate-rebuild`: active candidate, but should be rebuilt from selected
  commits because of merges, docs/spec contamination, or other branch-shape
  issues.
- `candidate-high-risk`: active candidate with a large or platform-sensitive
  surface; still valid, but not a quick first PR.
- `alias-only`: historical or convenience name; do not use as the PR source.
- `obsolete-or-deferred`: not in the current PR path unless future evidence
  reactivates it.

## Low-Level Design

### Record Storage

- The package stores one branch record per canonical candidate in
  `readiness-records/`.
- The filename matches the branch slug used in this package, with slash-named
  refs normalized into readable filenames such as
  `perf-image-plane-lifecycle-lazy-texture.md`.
- Shared structure lives in
  [readiness-record-template.md](readiness-record-template.md).

### Update Rules

- Refresh [research.md](research.md) first if `upstream/main` changes.
- Update the specific readiness record before touching the package-wide task
  state for that branch.
- Record alias-routing changes in [research.md](research.md), not inside an
  unrelated branch record.
- Put actual validation results in the relevant readiness record and summarize
  package-level validation only in [verification.md](verification.md).

## Readiness Record Schema

Each branch record follows the template in
[readiness-record-template.md](readiness-record-template.md) and must capture:

- canonical branch ref and any legacy aliases used by older package artifacts;
- current fetched baseline commit;
- current changed surface and history shape;
- cleanup plan for merges or unrelated docs/spec commits;
- issue and PR overlap placeholders;
- validation and manual evidence plan;
- reviewer-routing and human sign-off fields;
- blockers and next action.

## Dependency Model

- Package baseline tasks unlock all branch work.
- Candidate-matrix reconciliation must happen before readiness records can be
  trusted.
- Readiness-record creation must happen before a branch can advance to issue
  research or validation planning.
- Cleanup planning must precede any final PR-branch preparation.
- Human sign-off remains the final gate even after validation passes.

## Validation Strategy

- Package edits use:
  - spec-package lint
  - `git diff --check` limited to this package
- Branch readiness planning uses:
  - `git diff --check`
  - `pixi run build` by default, or a narrower justified command
  - focused tests matched to touched files
  - manual GUI/startup evidence when the change is user-visible
  - benchmark evidence for performance branches

## Independent Review Strategy

- GUI-visible changes may need Design Working Group awareness.
- Architecture-sensitive or cleanup-heavy changes may need Code Quality Working
  Group awareness.
- Startup and cross-platform branches may need Windows/macOS confirmation
  beyond local Linux evidence.
- Open GitHub issues and PR overlap must be recorded before a branch is treated
  as review-ready.

## Operational Considerations

- This package is allowed to describe branch cleanup work, but not to perform
  that work in this slice.
- Records should be updated branch-by-branch rather than reviving another large
  package-wide checklist.
- Branch names with slash-vs-hyphen drift must be treated carefully; the
  canonical name in the record wins.

## Risks

- A branch may remain technically valid but still be a poor first PR because it
  is far behind `upstream/main`.
- Performance branches with local docs/spec commits are especially prone to
  accidental scope leakage if rebuilt carelessly.
- Local package truth can drift again if a new fetch changes `upstream/main`
  and the matrix is not refreshed.
- Open AI-policy discussions may still affect how contributors choose to frame a
  later PR, even though the local policy baseline is already explicit.

## Open Questions

- Which active candidate should be the first branch-specific slice after this
  package revision?
- Which candidates still have enough upstream value to justify cleanup and PR
  preparation once overlap research is complete?
- Which branches can rely on existing test coverage, and which need new or more
  explicit manual evidence?

## Promotion Targets

- durable contributor workflow notes if this review process becomes recurrent
- historical closure or archive record when all currently tracked branches are
  routed
