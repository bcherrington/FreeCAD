# T004.2 corrected mapped-runtime diagnostic

Date: 2026-11-08. Owner: local-developer. Status: technical correction selected;
exploratory only, with unchanged D001 acceptance criteria.

## Closed attempts and identified cause

The first diagnostic freeze stopped at its first eager worker. Zero observations
qualified; the other 15 workers were not run. Its data, host trace, failure and
39,964-hash post-abort audit remain intact; it will not be resumed or pooled.

The separately reviewed two-process forensic qualification is complete. Both
eager and lazy receipts show exactly two additional mapped files:
`Mod/BIM/Arch_rc.rcc` and `Mod/Draft/Draft_rc.rcc`. There are no missing files or
changed hashes among the original 14 mappings. Both extra files match the complete
qualified build manifest. Both forensic audits pass 39,974 bindings. No forensic
timing values are accepted or reported.

These are Qt resource collections, not unexpected native libraries. The retained
P01 runtime includes BIM and Draft, and their `InitGui.py` scripts import their
resource loaders at workbench registration. The older T020 context omitted those
mapped resources. The probe's `runtime_module_sha256` map actually covers all
mapped build files; its exact-equality check correctly rejected this stale list.

## Concrete correction and bounded new diagnostic

Use the forensic observations to update the candidate's expected mapped-runtime
lists to the exact 16 known, hash-pinned files. Keep exact equality, core-library
path ownership, complete runtime/source/dependency/package audits and all existing
full model, byte, calibration, frame, viewport and native-profile witnesses.
Do not ignore RCC files or permit arbitrary additional modules.

The native C++/binaries remain the final 408-case qualified artifacts. The only
Python source change retains useful failure receipts; its native-source equivalence
is recorded. No new algorithm, product behavior, budget or acceptance waiver is
selected.

Start a **new independent** exploratory diagnostic in
`build/spec003-t004-p07-corrected-diagnostic`, with a new freeze. Run exactly 16 fresh
GUI workers: eager/lazy/lazy/eager for hidden, visible, PartDesign and VRML, in that
order. No extra qualification worker, retry, replacement, exclusion or statistical
acceptance assessment is permitted. Each worker enforces the corrected exact
runtime and full fidelity witnesses. Stop on the first unexpected failure and
report remaining coverage as unexecuted.

Use verified independent copies of the original untouched lazy-arm eight-file
Mesa/XDG seed, the same ordinary warm-filesystem policy, native X11 renderer,
viewport/DPR/MSAA, preferences, camera, fixture and addon identities. Preserve all
prior cache states and attempts. Host telemetry uses the tested, hash-bound host
Python under `-I -S -B`; native workers keep locked Pixi Python. Bind the launcher,
interpreter/module/native closure, absolute Pixi, collector and contexts. Verify
initial cache identity before sampling and record its expected mutation afterward.

Collect host telemetry at 1 Hz, beginning 30 seconds before the controller; keep
60-second/256 KiB worker limits and a 1200-second controller ceiling. Run no
concurrent agent build/test/native worker. The reviewed pidfd-owned process cleanup
remains mandatory. Publish only sanitized descriptive results and attribution
limits. This plan authorizes this one corrected 16-worker diagnostic, not another
D001 acceptance campaign.
