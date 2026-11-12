---
title: T010 final composition and evidence applicability
doc_type: spec
artifact_type: review
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# T010 Final Composition And Evidence Applicability

T010.1 selects final root `b5f15819217b9f450211caa589ddec10c285d40f` (the base source/tests/config were clean before the selected patch),
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

T010.1 completes exact base-plus-patch selection and affected-evidence routing.
T010.2 build/focused checks and T010.3 final full tests have passed on v3;
T010.4 supplies the completed runtime/manual/render packet and independent
review before guarded task completion. No pre-existing binary, pending result or source similarity is accepted
as execution evidence. Compose-profile Memcheck does not certify IBus; diagnostic
and normal builds remain distinct. No performance pair or build tree is added.

## Final Corrective Patch And Current Disposition

The v3 candidate is the committed base plus the exact five-file patch, not a
claimed new commit. The first full gate found pre-existing finite loft-edge
admission and Measure fixture initialization failures; the two-file repair passed
that full gate. Later isolated native warning traces exposed the App-only restore
gap and repeated preview hide. Two ready/late regression rows fail old code and
pass the v3 App restore guards; preview hide is now idempotent.

The final v3 build/focused/native/memory checks are fresh. Native tracing shows
six empty initialization calls, one nonempty first-show load, no App-restoring
loads, zero image/preview warnings, and successful private document/viewer
teardown. User-site modules are hidden in the validation process namespace and
actual loaded modules match the final build/patch. The final complete CTest graph
passed (2426 passed, zero failures, two skips and eight disabled); independent
packet review and the final hash-bound receipt complete the acceptance record.

P01/P07 historical external embedded-Python closure is unknown; the old
zero-premature classifier omitted App-only restore. Their exact numeric timing,
whole-open, tail and RSS values are historical. Retain only the reviewed local
directions and first-use/logical-memory tradeoff; current numeric production
claims need a new masked-runtime campaign. P02/P03 isolated workers and full maps
exclude this user-site concern. All original results remain unchanged.

[The full final report](t010-results.md) and its JSON distinguish every attempt,
source correction, active runtime bound, reviewed equivalence and limitation.
