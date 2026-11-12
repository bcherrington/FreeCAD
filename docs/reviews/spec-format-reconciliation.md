---
title: Active spec format reconciliation
doc_type: history
status: active
owner: local-developer
last_reviewed: 2026-11-06
---

# Active Spec Format Reconciliation

## Scope And Authority

Migrate the three active legacy packages before further implementation. Keep
`000-sketcher-diagnosis-performance` archived; only its backlink to the renamed
performance package is updated. The user requested
review and reconciliation to current formats. Repository templates remain the
authority, with a selective update based on installed Spec Lifecycle Manager
0.6.1 conventions. Structural validation does not establish implementation,
release, or closure readiness.

## Template Decision

Grade: selective template update. Affected files under
`docs/templates/spec-package/`: add `README.md`, `requirements.md`,
`traceability.md`, `verification.md`; update `design.md`, `tasks.md`, `spec.md`,
`plan.md`, `research.md`, and `quickstart.md`. Update `docs/templates/README.md`
to identify the current contract. Missing fields are artifact types, numbered
requirements, explicit acceptance, dependencies, evidence, promotion and
readiness. The legacy spec/plan entry points become optional brief/history
templates; they no longer define a second requirements/task source.

Only active spec packages migrate. Archived packages are grandfathered. Durable
document classes are unchanged. Documentation-authority review checks that
proposed feature behavior stays draft and source-derived observations retain
their revision and validation limits. No upstream policy or product acceptance
is implied by this local convention change.

## Package Mapping

| Previous path under docs/specs | Current package | Disposition |
| --- | --- | --- |
| cad-file-thumbnailing | [001-cad-file-thumbnailing](../specs/001-cad-file-thumbnailing/spec.md) | Draft; rendering/isolation decisions open |
| compact-view-rails-rework | [002-compact-view-rails-rework](../specs/002-compact-view-rails-rework/spec.md) | Draft; rail reintroduction and geometry decisions open |
| freecad-performance-optimization | [003-freecad-performance-optimization](../specs/003-freecad-performance-optimization/spec.md) | Partial implementation; benchmarks, visual checks and PR gates open |

IDs 001–003 were allocated after inventory found only historical ID 000.
Existing FR/PR/SC identifiers remain aliases. Legacy briefs, plans, task
checklists and measurements remain historical migration inputs. Current
requirements/design/tasks/traceability take precedence. No product code is
implemented as part of format migration.

## Evidence Boundary

Source baseline: `d0644236e41f35a849b12a1e77941aa88354496f`. Direct reads confirm
FCStd-only extraction, titlebar layout without rails, buffered property-file
restore, direct conversion for four 32-bit image formats, full-resolution lazy
image-plane loading, element-map token parsing and pending tree status updates.
The 2026-11-04 test results remain dated imported evidence; this migration does
not rerun or certify those binaries. Old trace switches and transient `/tmp`
artifacts are not reproducible current acceptance evidence.

The workspace contains a nested Git repository at `docs/`; both repositories
use source branch `docs/reconcile-spec-formats` for this work. Existing dirty
documentation was preserved before migration. The standalone-console design is
outside this spec-format change.

## Validation And Review

Completed checks on 2026-11-06:

- Spec Lifecycle Manager scan: three active current-format packages pass, zero
  diagnostics; archived package excluded. Numbering inventory now proposes 004.
- Individual package lint: zero errors, warnings or information findings for all
  three active packages. Nine spec artifact templates also lint with zero findings.
- Direct documentation check: 42 documents, 173 local file/anchor links,
  28 requirements, 22 parent tasks and 44 subtasks; required fields, dependencies,
  traceability and legacy FR/PR/SC aliases resolve.
- Compared legacy inputs against pre-migration working copies: content retained
  except explicit historical metadata/notice and renamed-package links. Archive
  changed only at its navigation backlink. All 37 original performance checklist
  entries remain in legacy-tasks.md with a disposition map to the current queue.
- Root and nested-docs `git diff --check` pass. The configured pre-commit path
  filter excludes docs, so an all-files run is not an applicable documentation
  check. No FreeCAD build/runtime tests were rerun for this documentation change.
- Bounded task_context reads resolve the next T001 requirement/design/evidence
  context in all three packages without traceability gaps.

Task-state audit advisories were reviewed, not used as acceptance evidence.
They flag recorded absence/partial evidence as candidate completion and broad
parent checkpoints as possible splits. States remain truthful; broad gates now
have evidence-bearing subtasks. A task mentioning future routing is not marked
routed before a destination is actually recorded. The reconciliation tool reports
unfinished tasks and cannot interpret C++/Python sources as durable Markdown;
direct source reads provide only the implementation-presence evidence stated
above.

Agent Workbench validation planning is blocked by the nested docs repository
boundary (no .gitmodules traversal authority). It planned/executed no checks;
explicit lifecycle calls and direct checks above supply the validation. Its stale
index is not used to establish current acceptance.

### Documentation Review Disposition

Codex performed a bounded authoring/authority review of the resulting artifacts;
this is self-review, not independent maintainer or product approval. Current
requirements have observable criteria and useful user outcomes. Existing
performance plan behavior is now explicit in Requirements 4–6 rather than hidden
in an untraceable checklist. Historical measurements and proposals remain labeled,
review-required decisions block dependent work, and promotion destinations are
identified as proposed rather than existing current guidance. No documentation
format blocker remains. Product decisions, implementation validation and expert
review of the eventual feature changes remain open in the packages.

No commit, integration merge, push, feature implementation or lifecycle closure
was performed. Changes are ready for local review on the source branches.
