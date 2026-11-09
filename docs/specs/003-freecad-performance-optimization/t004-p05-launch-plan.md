---
title: P05 frozen activation and direct namespace launch
doc_type: spec
artifact_type: execution-plan
status: selected-for-construction-and-qualification
owner: local-developer
last_reviewed: 2026-11-08
---

# P05 Frozen Activation

This prospective launcher refinement applies only to P05, after P04 closes.
The [conversion plan](t004-conversion-plan.md) remains authoritative for fixtures,
algorithms, timing endpoints, correctness, sampling and D001. No samples are pooled
between launcher routes or comparisons.

Activate the exact locked Pixi environment once using argv, project cwd and
`/usr/bin/env -0`. Select only PATH, LD_LIBRARY_PATH, locale, DISPLAY, XAUTHORITY,
XDG_RUNTIME_DIR and fontconfig variables that exist. Do not retain other operator
environment variables or credentials. Keep values and XAUTHORITY locators private;
do not record the authentication cookie. Bind the selected mapping's canonical
JSON hash, activation command/exit status, exact Pixi/version/manifest/lock, resolved
native Python executable and its imported stdlib/native closure, and resolved git.

Each fresh worker launches this native Python directly with `-I -S -B` into the
same qualified read-only namespace. The inner checker also uses `-I -S -B`.
Build each environment from the frozen allowlist, adding only fixed per-arm HOME,
XDG configuration/cache, Mesa cache, FreeCAD homes, native XCB selection and Python
isolation settings. Do not inherit the controller's operator environment. Runtime
configuration and caches remain independent between arms and are audited.

Before campaign artifacts, the controller verifies its actual resolved interpreter
and frozen binary hash, callable pidfd APIs and a real self-pidfd signal-zero plus
subreaper initialization/restoration. Validate activation hash/key allowlist,
Python/git identities, exact qualified worker argv and all original source/runtime,
fixture, schedule and destination guards. Qualify XCB/runtime maps and full pixel,
orientation/storage/ownership checks under this final route before its new freeze.

Review accepted this as removal of irrelevant repeated environment activation,
not a change to native conversion timing, original sampling or numeric acceptance.
The P04 controller/freeze/raw results stay unchanged; its explicit operator launch
and independently checked live executable/argv/start identity provide reviewed
actual-controller provenance. Its supplemental receipt and original host-closure
binding belong in final validation. P05 adds the automatic interpreter preflight.

The selected receipt has the exact schema, integer exit status zero, exact capture
argv/cwd and zero stderr with its empty-content hash. Validate these before any
campaign artifacts. Perform the full receipt/executable/environment validation
once per campaign; per-worker environment construction is a pure copy. Audit all
files beneath each writable cache root, including HOME/config/data/temp, and reject
file or directory symlinks. Record legitimate mutations separately after closure.
