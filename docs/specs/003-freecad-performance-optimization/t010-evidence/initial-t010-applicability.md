---
title: T010 final composition and evidence applicability
doc_type: spec
artifact_type: review
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# T010 Final Composition And Evidence Applicability

T010.1 selects final root `b5f15819217b9f450211caa589ddec10c285d40f` (source/tests/config clean),
with seven clean matching recursive dependencies, unchanged locked 510-package
environment and all nine pinned fixtures verified. The unrelated console-design
edit is excluded. This is the integrated branch, including separately completed
Sketcher reattachment work and GuiDocument queued-update correction; it is not
claimed to be an isolated upstream performance-only PR.

The owner selected **Linux-only acceptance** on 2026-11-09. Windows, macOS,
Wayland and other GPU/driver combinations remain unqualified. The accepted
validation input-method profile is compose (`QT_IM_MODULES=compose`,
`QT_IM_MODULE=compose`); IBus memory-access qualification is explicitly excluded.
Execution receipts must prove the environment and loaded module profile. D003 final scope
is this integrated Linux composition. Any later cherry-pick, source/dependency,
build/runtime, renderer or fixture change needs affected requalification or a
new reviewed equivalence; branch name or old timing assembly is insufficient.

[Final source manifest](t010-final-source-manifest.json) records exact hashes.
[Machine-readable reconciliation](t010-applicability.json) enumerates every
correctness group and P01–P08 campaign composition/delta. Prior T004.4 authority
is preserved byte-exact in [the archive](t010-prior-authority/manifest.json).

## Requalification And Equivalence Plan

| Area | Final disposition and required proof |
| --- | --- |
| Archive/map | Original safe implementation and bytes/error semantics retained; guarded tracing does not prove final binaries. Run final native archive/Reader/map tests and dependent CAD save/reopen/recompute. |
| Image lifecycle/render | T006 alone is insufficient after later lifecycle/error fixes. Use later T014/T020 qualification and rerun current full calibration/import/hidden-show/error, pixel/render suites and bounded lifecycle Memcheck. |
| GUI/tree | Tree implementation unchanged since T017/T007; GuiDocument stale queued-property change is functional. Run new DocumentUpdates plus tree/lifetime/selection integration on final libraries. |
| Sketcher/GCS | Integrated independent changes require final Sketcher/recompute/reattachment checks; no automatic whole-open binary or timing equivalence. |
| Performance | Preserve original D001 measurements and D007 selected scopes. Review unchanged local algorithms/trace guards, dependency/fixture identities and final functional proof; disclose different campaign builds/profiles. No claim of fresh or identical final whole-workflow timings. New sampling only for an actual applicability gap, never an old numeric veto. |

T010.1 completes composition selection and explicit affected-evidence routing.
Final acceptance remains gated by T010.2 build/focused checks, T010.3 full tests,
and T010.4 executed functional/manual/render proof plus independent equivalence
review. No pre-existing binary, pending result or source similarity is accepted
as execution evidence. Compose-profile Memcheck does not certify IBus; diagnostic
and normal builds remain distinct. No performance pair or build tree is added.
