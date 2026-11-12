---
title: Documentation branch consolidation, 12 November 2026
doc_type: history
status: active
owner: local-developer
last_reviewed: 2026-11-12
---

# Documentation Branch Consolidation

## Scope and preservation

User requested committing the current docs checkout, comparing latest local branch commits,
and consolidating/cleaning diverged branches. The docs repository has no remote, so this audit
covers all six local heads; no remote state is inferred. The Assembly implementation/tests remain preserved on feature/assembly-motion-suspension.
A later parent-repository audit below explains its separately versioned docs snapshot.

The existing docs/reconcile-spec-formats working tree was committed as `91b92db` before any
reconciliation. It contained 480 changed tracked paths (13 modified and 467 deleted) plus six
legacy Markdown files that Git recognized as moves/rewrites. Generated Python caches were
excluded; .gitignore now prevents them appearing as delivery work.

## Initial branch comparison

Ahead/behind counts below are relative to spec/assembly-motion-suspension at 0ec8532.

| Branch | Initial tip | Tip date | Commits only on this branch | Commits only on assembly docs | Disposition |
| --- | --- | --- | --- | --- | --- |
| async-recompute-current-status | f720ec3 | 2026-06-06 | 0 | 140 | Fully included; safe to prune after consolidation |
| main | 19715d5 | 2026-08-14 | 0 | 71 | Fast-forward to consolidated docs |
| spec/001-007-lifecycle-migration | 9ffd147 | 2026-11-04 | 0 | 49 | Fully included; safe to prune |
| perf/sketcher-diagnosis | 61792d0 | 2026-11-05 | 0 | 37 | Fully included; safe to prune |
| docs/reconcile-spec-formats | 91b92db | 2026-11-12 | 1 | 5 | Preserve checkpoint; reconcile rollback, then merge |
| spec/assembly-motion-suspension | 0ec8532 | 2026-11-12 | 0 | 0 | Merge all five commits; remove redundant clean worktree |

The only divergence was checkpoint91b92db versus five Assembly spec commits; common ancestor
87965c6. The migration, performance and async branch heads were already reachable from both.
Older branches did not need independent merges or cherry-picks.

## Content findings and decisions

The checkpoint removed the numbered 001–005 packages, spec index, authoritative requirements,
traceability and verification templates, closed-delivery records, references and retained evidence.
Edited dates moved backwards; completed/current reconciliations became older proposed work.
The six unnumbered files matched historical legacy inputs with newer reconciliation notices
removed. Python Console behavior likewise reverted from November code-derived reconciliation
to May design proposals. No newly authored requirement was identified for forward-porting.

The checkpoint remains reachable in history. Correction `43a6cea` reverses its regression;
its committed tree was checked byte-for-byte against canonical87965c6 via Git tree IDs.
Merge `ca25051` includes both that preserved checkpoint/correction and Assembly delivery0ec8532.
The merged content was checked identical to0ec8532 before adding this report, cache exclusions
and a small archive-index repair. No spec evidence or valid feature commit is discarded.

## Validation and cleanup rules

- SLM scan: seven numbered packages; three active (001,002,005), all pass with zero warnings/
  errors; four retained histories (000,003,004,006).
- Archive validation found one006 destination missing from the index (AssemblyObject.pyi),
  now reconciled with its closure log. Recheck recorded after the repair.
- Verify all six initial tips and checkpoint91b92db remain ancestors of consolidated main.
- Fast-forward main; prune branch names only with Git's merged-branch safety check.
- Remove the Assembly worktree only while clean and after all its commits are on main.
- Do not force-delete branches/worktrees or erase generated caches; caches are ignored.

This is a Git/documentation reconciliation, not a rerun of CAD code tests or evidence claims.
Earlier qualified source/build identities and historical failures remain unchanged.

## Completed consolidation

- Checkpoint `91b92db`, correction `43a6cea`, merge `ca25051`, and audit/archive repair
  `e573549` are reachable from docs/main.
- docs/main was fast-forwarded with --ff-only; no history was rewritten.
- All five other local branch names were removed with `git branch -d` after explicit
  ancestor checks: async-recompute-current-status, perf/sketcher-diagnosis,
  spec/001-007-lifecycle-migration, docs/reconcile-spec-formats and
  spec/assembly-motion-suspension.
- The redundant FreeCAD-docs-assembly-motion worktree was verified entirely clean (including
  untracked files) and removed without force. Its complete contents are now in FreeCAD/docs.
- Generated caches remain on disk and are ignored. No source implementation, installed runtime, remote or historical evidence was deleted
  or modified during the nested-repository cleanup. Parent docs refs are audited below.
- Final validation: SLM active-spec scan3/3 pass, four retained packages; archive-index
  check4/4 consistent with zero errors/warnings; canonical package paths/templates restored.

Current documentation checkout is FreeCAD/docs on main. Previous worktree links should be
replaced by the corresponding path under this checkout. Earlier branch names in immutable
spec evidence describe their original delivery context, not additional unmerged work.

Targeted link validation checked20 canonical/review/template documents and89 local links;
all targets exist. Git whitespace validation passed. No CAD build/test rerun was needed for
this documentation-only consolidation.

## Parent FreeCAD repository: second ownership layer

The final parent status check revealed58 regular tracked docs files in the source repository.
Its old templates have the same Git blob as checkpoint91b92db. Source branch checkouts can
therefore replace newer nested-repository content with an older parent snapshot. This explains
why completed standards migration could appear undone despite being committed in docs Git.
The nested docs repo is clean; that alone does not establish a clean parent repo.

| Parent source branch | Tip at audit | Unique commits vs Assembly feature | Non-docs paths touched by unique commits | Decision |
| --- | --- | --- | --- | --- |
| docs/reconcile-spec-formats | 68556525ac | 34 | 70 | Retain; separate code-integration review required |
| docs/spec-lifecycle-reconciliation | 7469a6ec53 | 10 | 0 | Preserve ancestry with current canonical docs content |
| docs/remove-legacy-performance-spec | 8b51d0d5a8 | 0 | 0 | Already merged; safe to prune |

The34 commits include App/Gui performance, Sketcher reattachment and native test changes;
the name docs/reconcile-spec-formats does not make it documentation-only. Its docs subtree
matches nested87965c6 in every path/blob except the older May Python Console proposal.
Its source changes are outside this documentation consolidation and are not blindly merged.

The exact docs subtree of parent7469a6ec53 exists as nested commit1574c76fc714cab68e2271c54899679fba2137d6,
which is an ancestor of nested main. All ten unique parent commits change docs paths only.
An ancestry-preserving merge can therefore retain its history with the newer canonical snapshot,
without reviving older formats or changing CAD code.

Parent branch docs/consolidated-snapshot is based on the Assembly feature, preserving its source
content and keeping the original feature branch separate. Its docs subtree is synchronized to
nested main using the exact nested tracked-file list (no .git metadata or caches). No repository
ownership conversion or untracking policy is introduced by this task. Until dual ownership is
changed deliberately, use separate source worktrees or synchronize the parent docs snapshot
before source-branch switches in the canonical checkout.

## Parent snapshot completion and remaining divergence

Parent snapshot `cc7b6227943` has a docs subtree exactly equal to nested main at capture time;
individual tracked paths were staged explicitly, excluding repository metadata and caches.
History merge `09dc8f55e8d` incorporates docs/spec-lifecycle-reconciliation with no content change
(tree IDs before/after match). The older lifecycle and remove-legacy-performance-spec branch
names were then removed with safe `git branch -d`; their commits remain reachable.
Assembly feature branch feature/assembly-motion-suspension is retained separately. Parent
source content outside docs is byte-identical to that feature branch.

Root docs/reconcile-spec-formats at68556525ac is deliberately retained: its34 unique commits
include70 non-docs paths and need source integration review. This audit does not establish
whether every native change should be merged or whether platform/build regression gates pass.
Only the nested documentation branch set is completely consolidated to main; the remaining
parent code-bearing history is an explicit follow-up, not deleted or silently declared merged.

The parent snapshot preserves31 whitespace diagnostics in historical patch/text evidence;
normalizing those archived bytes was outside this reconciliation. Scoped newly authored docs
pass whitespace validation, and the synchronized subtree matches the nested authoritative Git
content. No CAD source validation is implied by this docs-only snapshot.
