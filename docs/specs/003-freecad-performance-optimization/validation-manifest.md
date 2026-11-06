---
title: Spec 003 candidate validation manifest
doc_type: spec
artifact_type: validation-manifest
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Candidate Validation Manifest

## Decision And Scope

**Manifest ID:** `spec003-linux-x86_64-debug-candidate-20261106-v1`. **Decision:** D003 initial local candidate and
applicability policy resolved on 2026-11-06 under the user's instruction to
complete T019. Codex gathered evidence and made this delegated technical
selection; it does not claim independent human, maintainer or platform approval.

Select root `f7fe2520c2edf25a17f3ad1fde084116cb1627d4` and its seven clean matching recursive Git dependencies
as the initial local integrated candidate before the corrective source tasks.
The spec intake revision is `da79497b9b4ee447fbd8e5b0cde168af55b47118` in the nested docs repository. This
pins the current source, not a clean upstream PR base or a performance baseline
for every historic optimization. D001 must still choose each comparative
baseline and protocol before accepting measurements.

Use Linux x86_64, the locked Pixi environment and `conda-linux-debug` initially
for correctness and diagnostics. Use Qt offscreen for bounded unit checks and
native X11 for explicit rendered/manual evidence. Native GUI results require a
captured backend/renderer/driver receipt at execution; DISPLAY availability
alone is not graphics evidence. Windows, macOS, Wayland and other GPUs/drivers
remain unverified. T010 retains supported-platform and exact-final-PR gates.

## Machine-Readable Snapshot

[validation-manifest.json](validation-manifest.json) is the immutable v1 intake
snapshot, generated from direct Git, SHA-256, CMake-cache, tool-version and
installed-package metadata reads. It includes fixture/source identities and
planned fixture recipes; it contains no runtime acceptance results.

- SHA-256: `2ceaf3dbe3d474f4bb8d70ae6706218ddffdd3616473a1c13a55133611a19d01`.
- Installed package identities: 510; normalized identity SHA-256:
  `27048ce62e5ad835811cb32e58d11ee775f304c81bfef5235bbdd397c3e59d28`.
- Root source/test/config worktree: clean at capture. The pre-existing standalone
  console documentation edit is excluded and preserved. This task's subsequent
  documentation changes do not imply application source changes.
- Root `src` tree: `5d04131f65f14d62edf08d265bc470c6d701f7e9`.
- Root `tests` tree: `5c9202766988239f418c04f990257c467a26ea86`.
- Root `cMake` tree: `8c56e6aa74882f8181f05d5ecd03ce4d2a94f8a2`.
- Pixi lock SHA-256: `45727758e595efad731c9bc71e429719040b1509ad686d762ad52ba84e44ec5c`.

The JSON records all package name/version/build/archive checksums, root config
and selected implementation/test file hashes. Installed package metadata is
not proof that every installed payload file is untouched. Accepted runtime
receipts must additionally identify the rebuilt binaries and resolved runtime
libraries. Do not infer source provenance from file timestamps/version strings.

## Git Dependency Composition

Root `.gitmodules` declarations and parent gitlinks were checked directly;
all seven checked-out heads match their recorded parent revisions and have no
tracked or untracked changes. The earlier stale Workbench declaration-only
view is not used to establish this composition.

| Path | Commit | State |
| --- | --- | --- |
| `src/3rdParty/GSL` | `a75212b9f3b14162edd62d540cbf9273d5a59d20` | Clean; parent gitlink matches |
| `src/3rdParty/OndselSolver` | `4be80eef02a3486cda0d78f3ccbb308d207a9639` | Clean; parent gitlink matches |
| `src/3rdParty/coin` | `50e37f04e7027661ea2318d0425b7836ac819568` | Clean; parent gitlink matches |
| `src/3rdParty/coin/cpack.d` | `118ac5a21bcf57f0f90e2b0e681c9dcbf07074c2` | Clean; parent gitlink matches |
| `src/3rdParty/coin/docs/doxygen-awesome` | `6217657eef6aec24a0e4ecea4695031c1a622cf3` | Clean; parent gitlink matches |
| `src/3rdParty/pivy` | `515f038d32f163a976acd8a8cd9e8d26e1d0b884` | Clean; parent gitlink matches |
| `src/Mod/AddonManager` | `745c1b1ebc8bc684439210c5fa237d2c6287153b` | Clean; parent gitlink matches |

FastSignals and zipios++ are root-vendored trees rather than independently
versioned checkouts. Their root tree IDs and file identities are in the JSON.
Do not substitute external Coin/Pivy or another parser dependency silently.

## Build And Binary Eligibility

| Item | Selected or observed value | Acceptance meaning |
| --- | --- | --- |
| Host | Linux Mint 22.3, Linux x86_64, kernel 7.0.0-42-generic | Initial local scope only |
| Configure preset | `conda-linux-debug`; Ninja; `build/debug` | Current observed configuration and selected initial profile |
| Build | `pixi run --locked build -j 2` | Required on the actual candidate/fix composition before new runtime evidence |
| Full tests | `pixi run --locked test` | Required final gate as declared by T010; not executed by T019 |
| Compiler | Clang 21.1.8; C++ Debug `-g`; mold linker | Effective cache/compiler receipt captured; compare like profiles |
| Tool versions | Pixi 0.83.0; CMake 4.4.3; Ninja 1.13.2; Python 3.13.18 | Tool executable identities captured in JSON |
| Principal dependencies | Qt/PySide 6.11.3; OCCT 8.0.1; Boost 1.90.0; GTest 1.18.0 | Exact package builds and archive hashes are in JSON |
| Coin/Pivy | Bundled; external Coin/Pivy OFF | Git revisions above, not an inferred system package |
| Sanitizers | ASan and UBSan OFF in observed normal Debug cache | T012/T015 must obtain separate sanitizer/equivalent evidence; no safety result claimed |
| Eight existing executable/library artifacts | SHA-256, ELF build IDs, DT_NEEDED and RPATH/RUNPATH recorded | Historical, source composition unverified; all `eligible_for_new_acceptance=false` |

The observed cache and compiler identification were hashed; stale compiler
identification directories also exist. Only the cache-selected 4.4.3 receipt
is used for the observed configuration. No build was run during T019, and no
existing binary is promoted to acceptance merely because its configuration
looks compatible. A successful new build must have an execution receipt tying
its resulting executables/shared libraries, source/dependency/worktree state,
actual flags and resolved runtime libraries together.

Debug is the initial correctness/diagnostic profile. D001 must select matched
optimized builds before release-performance claims. Instrumented/sanitized
builds use separate directories/receipts and do not share normal-build timing
samples. Reconfiguration or a Pixi environment/lock change invalidates affected
binary and performance provenance until a new receipt is captured.

Pixi 0.83.0 `run --help` was inspected: `--locked` aborts when the lock is
out of date instead of silently updating it. The selected commands are the
normal repository Pixi configure/build/test tasks with this reproducibility
flag. They have not been executed by T019. Abort and explicitly reconcile any
lock/environment change before accepting later results.

## Fixture Inventory

The JSON records SHA-256 and byte size for each of these nine actual files.
The eight repository controls can be located from the pinned root revision;
the external file is local-only and is not copied or distributed by this spec.
A fixture's presence/hash is not proof that a runtime case passed.

| Fixture ID | Locator | Selected role |
| --- | --- | --- |
| BIMExample | `data/examples/BIMExample.FCStd` | Large Part/BIM map and GUI restore workload |
| ArchDetail | `data/examples/ArchDetail.FCStd` | Large Part/BIM map and GUI restore workload |
| EngineBlock | `data/examples/EngineBlock.FCStd` | Small geometry/map restore control |
| PartDesignExample | `data/examples/PartDesignExample.FCStd` | PartDesign feature/subelement reference control; target checks still to be specified by T016 |
| draft_test_objects | `data/examples/draft_test_objects.FCStd` | Draft restore control |
| AssemblyExample | `data/examples/AssemblyExample.FCStd` | Assembly/dependency restore control |
| ProjectTest | `data/tests/ProjectTest.FCStd` | Existing App document compatibility fixture |
| TestVRMLTextures | `data/tests/TestVRMLTextures.FCStd` | Existing texture/document control; not calibrated-plane acceptance evidence |
| gridfinity-drawer-v1.3 | `FREECAD_BENCH_FILE` (must match recorded SHA-256) | Historical large-image document comparison candidate; runtime method and D001 pass rules remain pending |

The current PropertyFile/Reader/ElementMap/BitmapFactory unit-fixture source
files are also hashed. Future extent/calibration/numeric/identity/failure cases
have deterministic scenario/recipe IDs in the JSON, with recipe hashes. Their
output hashes are deliberately null and status is `planned-unmaterialized`:
T012–T018 must materialize/version them and hash actual outputs or generator/test
source before accepting their first result. A recipe is not a passed fixture.
If the external large-image file is unavailable or changes, do not substitute
another document under the same ID; obtain a new manifest/fixture revision.

## Required Execution Receipt

Every dependent source/correctness/measurement task must reference this manifest
ID and append a versioned execution receipt to verification evidence with:

1. Actual root commit and any working-diff digest, recursive dependency heads
   and cleanliness, Pixi lock and installed-package identity.
2. Configure preset/effective flags, tool versions and platform, successful
   build command/result/log, executable and core library hashes/build IDs, and
   resolved runtime library composition. A source fix changes the original
   candidate; record the actual resulting composition rather than reusing v1's
   original implementation fingerprint.
3. Fixture ID/hash, or recipe and materialized output/test-source identities;
   command/workflow, acceptance criteria, result and limitations. GUI evidence
   additionally records backend/renderer/driver and display conditions.
4. Evidence applicability revision and any explicitly reviewed equivalence.
   D001 additionally governs comparative endpoints, sampling and pass rules.

Use named source branches for corrections. Preserve the unrelated console docs
edit. Rebuild through the normal Pixi tasks on the actual chosen composition;
never reuse old included-file/image/manual results solely because tests built
successfully later. No source correction or runtime acceptance is authorized
by an advisory context-ready response overriding the task's recorded gates.

## Invalidation And Final Confirmation

| Change | Required disposition |
| --- | --- |
| Source/tests, recursive dependency head/dirty state, build flags/toolchain, environment or relevant runtime libraries | Rerun affected automated, archive, calibration/lifecycle, visual/manual and performance cases, or record an explicit reviewed equivalence tied to the exact final composition |
| Fixture bytes, generator/test recipe, visibility state or private fixture availability | Invalidate affected results; version/hash the new fixture; no silent substitution |
| Renderer/backend/driver, instrumentation or profiling/build mode | Rerun affected rendered/performance/resource checks; separate populations and attribution limits |
| Documentation-only commit | Retain evidence only with recorded scoped equivalence showing relevant implementation/test/config/dependency/fixture hashes unchanged |
| Final cherry-pick, rebase or upstream patch selection | T010.1 compares exact final revisions and reruns/reconciles every affected result before T010 acceptance; earlier build-only success or timing assembly is insufficient |

This v1 intake snapshot stays historical when source fixes advance. Record new
receipts/manifest versions without rewriting a past result's provenance. D003
resolves the initial candidate and evidence policy only; T010.1's final
production confirmation, D001 benchmark protocol, all source fixes/regressions
and required platform coverage remain pending. This is not a release/closure
approval or a claim that an affected runtime test now passes.

## Recheck Commands

Run from the root repository. All commands below inspect metadata; build/test
commands above describe later acceptance work and were not run for T019.

```sh
git rev-parse HEAD
git rev-parse HEAD:src HEAD:tests HEAD:cMake
git submodule status --recursive
sha256sum pixi.lock CMakePresets.json pixi.toml
sha256sum docs/specs/003-freecad-performance-optimization/validation-manifest.json
```

Compare fixture/binary hashes against the JSON entries using `sha256sum` on the
listed repository paths. For the private fixture, hash the user-supplied
`FREECAD_BENCH_FILE` and compare the recorded identity; do not publish its local
path or model contents. Installed-package comparison uses the documented JSON
normalization algorithm over the same whitelisted conda-meta fields. Any
mismatch is evidence to reconcile, not permission to update a past result.

## Related Artifacts

- [Machine-readable intake snapshot](validation-manifest.json)
- [Requirements](requirements.md)
- [Design](design.md#evidence-applicability-and-benchmark-contract)
- [Decision D003](open-decisions.md#resolved-decisions)
- [Tasks](tasks.md)
- [Verification](verification.md#t019-validation-manifest)
