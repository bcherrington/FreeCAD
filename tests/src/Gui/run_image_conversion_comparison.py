#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Bounded, frozen Spec003 P04/P05 collector; no retries or policy changes."""

import argparse
import hashlib
import json
import os
import signal
import shutil
from pathlib import Path
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))

from run_image_diagnostic import audit as base_audit, collect, digest, require
from benchmark_host_telemetry import OwnedSession


def write(path, value, exclusive=False):
    with Path(path).open("x" if exclusive else "w") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")


def mappings(text, arm, canonical):
    result = set()
    for line in text.splitlines():
        fields = line.split(None, 5)
        if len(fields) != 6 or not fields[5].startswith("/"):
            continue
        name = fields[5]
        require(not name.endswith(" (deleted)"), "Deleted runtime mapping")
        if name.startswith(canonical + "/"):
            name = arm["source"] + name[len(canonical) :]
        result.add(name)
    return sorted(result)


def identity(row):
    keys = [
        "recipe",
        "width",
        "height",
        "source_format",
        "source_format_name",
        "source_bytes_per_line",
        "source_bytes",
        "source_has_alpha",
        "source_color_space",
        "source_color_space_valid",
        "source_pixel_sha256",
        "source_palette_sha256",
        "qt_version",
        "qt_platform",
    ]
    return {key: row[key] for key in keys}


def schedule(comparison):
    require(comparison in ("P04", "P05"), "Unknown comparison")
    cells = (
        ["raster-a", "raster-b", "raster-c"]
        if comparison == "P04"
        else [
            f"{recipe}-{extent}"
            for extent in [4096, 512]
            for recipe in ["indexed-gray", "indexed-gray-alpha", "premultiplied", "gray16", "svg"]
        ]
    )
    return [
        (cell, pair, arm, "diagnostic" if pair == 0 else "warmup" if pair < 4 else "measured")
        for cell in cells
        for pair in range(24)
        for arm in (["baseline", "change"] if pair % 2 == 0 else ["change", "baseline"])
    ]


def validate_cells(freeze):
    rows = schedule(freeze["comparison"])
    require(set(freeze["cells"]) == {r[0] for r in rows}, "Missing/extra fixture cell")
    payloads = {
        "raster-a": (
            "8dea3a5aafcbd33719a87142049fd2025efd72f21d0eea08cde4c9edef97a7ec",
            26002802,
            5712,
            4284,
        ),
        "raster-b": (
            "54d6565fab1d7d8949cd0c6fcdba5e2b453212990663144484a768c4f8c3304d",
            25753036,
            5712,
            4284,
        ),
        "raster-c": (
            "a500a37865628fda3b26894581f0d6c1150b64a380f66214a8acdfcd57f36784",
            5490205,
            1785,
            1959,
        ),
    }
    formats = {
        "indexed-gray": "Format_Indexed8",
        "indexed-gray-alpha": "Format_Indexed8",
        "gray16": "Format_Grayscale16",
        "premultiplied": "Format_ARGB32_Premultiplied",
        "svg": "Format_ARGB32_Premultiplied",
    }
    components = {
        "indexed-gray": 1,
        "indexed-gray-alpha": 2,
        "gray16": 2,
        "premultiplied": 4,
        "svg": 4,
    }
    direct = {"Format_RGB32", "Format_ARGB32", "Format_RGBX8888", "Format_RGBA8888"}
    for name, cell in freeze["cells"].items():
        require(set(cell["qualified"]) == {"baseline", "change"}, "Missing arm qualification")
        a, b = (cell["qualified"][arm] for arm in ["baseline", "change"])
        require(a["identity"] == b["identity"], "Arm source image identities differ")
        image = a["identity"]
        require(image["qt_platform"] == "xcb", "Wrong Qt platform")
        if freeze["comparison"] == "P04":
            sha, size, width, height = payloads[name]
            path = Path(cell["input"])
            require(cell["recipe"] == "payload" and cell["extent"] == 0, "P04 recipe changed")
            require(path.stat().st_size == size and digest(path) == sha, "P04 payload changed")
            require(freeze["file_bindings"].get(str(path)) == sha, "Unbound payload locator")
            require(
                (image["width"], image["height"]) == (width, height)
                and image["recipe"] == "payload"
                and image["source_format_name"] in direct,
                "P04 source format/dimensions changed",
            )
            require(a["coin_components"] == b["coin_components"] == 4, "P04 components changed")
        else:
            recipe, extent_text = name.rsplit("-", 1)
            extent = int(extent_text)
            require(
                cell["recipe"] == recipe and cell["extent"] == extent and cell["input"] == "-",
                "P05 recipe/extent/input changed",
            )
            size = "large" if extent == 4096 else "small"
            require(
                cell["refinement_id"] == "spec003-p05-materialized-" + size + "-20261108-v1",
                "Wrong refined recipe identity",
            )
            require(
                (image["width"], image["height"]) == (extent, extent)
                and image["recipe"] == recipe
                and image["source_format_name"] == formats[recipe],
                "P05 source identity/format changed",
            )
            require(
                a["coin_components"] == components[recipe] and b["coin_components"] == 4,
                "P05 historical/current component semantics changed",
            )


def host_snapshot():
    start = time.monotonic_ns()
    value = {"monotonic_ns": start}
    for name in ["stat", "loadavg", "meminfo", "pressure/cpu", "pressure/memory", "pressure/io"]:
        try:
            value[name] = (Path("/proc") / name).read_text()
        except OSError:
            value[name] = None
    value["observer_wall_ns"] = time.monotonic_ns() - start
    return value


def worker_command(freeze, cell, arm):
    item = freeze["arms"][arm]
    config = freeze["cells"][cell]
    prefix = [freeze["pixi_executable"], "run", "--locked", "python"]
    if freeze["comparison"] == "P05":
        prefix = [freeze["native_python"]]
    return prefix + [
        "-I",
        "-S",
        "-B",
        freeze["namespace_wrapper"],
        "--isolated-python",
        "--physical-source",
        item["source"],
        "--canonical-source",
        freeze["canonical_source"],
        "--cache-dir",
        freeze["initial_caches"][arm]["root"],
        "--commit",
        item["commit"],
        "--",
        freeze["canonical_source"] + "/build/relWithDebInfo/tests/ImageConversionPerformanceProbe",
        config["recipe"],
        str(config["extent"]),
        config["input"],
    ]


def run_worker(freeze, cell, arm, output_path):
    item = freeze["arms"][arm]
    cache = freeze["initial_caches"][arm]["root"]
    if freeze["comparison"] == "P05":
        env = worker_environment(freeze, arm)
    else:
        env = dict(
            os.environ, PYTHONDONTWRITEBYTECODE="1", PYTHONOPTIMIZE="0", QT_QPA_PLATFORM="xcb"
        )
        env.update(
            FREECAD_USER_HOME=cache + "/home",
            FREECAD_USER_DATA=cache + "/data",
            FREECAD_USER_TEMP="/tmp",
        )
        for name in ["QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "LIBGL_ALWAYS_SOFTWARE"]:
            env.pop(name, None)
    # Native exe is direct child of namespace wrapper; bounded owned-session helper.
    command = worker_command(freeze, cell, arm)
    collect(command, env, 60, Path(output_path))
    raw = Path(output_path).read_bytes()
    marker = b"IMAGE_CONVERSION_RESULT "
    rows = [line[len(marker) :] for line in raw.splitlines() if line.startswith(marker)]
    require(len(rows) == 1, "Missing/duplicate conversion result")
    return json.loads(rows[0]), raw


def validate_row(row, expected, arm, canonical):
    require(row["schema"] == "freecad-image-conversion-observation/v1", "Worker schema changed")
    require(
        row["correctness_passed"] is True
        and row["max_rgba_error"] <= 1
        and row["owns_pixels_after_source_write_and_destruction"] is True,
        "Pixel/ownership qualification failed",
    )
    require(identity(row) == expected["identity"], "Fixture source identity changed")
    require(row["coin_components"] == expected["coin_components"], "Component semantics changed")
    require(row["coin_pixel_sha256"] == expected["coin_pixel_sha256"], "Coin pixel output changed")
    require(
        mappings(row["runtime_maps"], arm, canonical) == expected["runtime_mappings"],
        "Loaded runtime mapping differs from qualification",
    )
    require(
        row["trace"]["storage_trace_balanced"] is True
        and row["trace"]["live_coin_bytes"] == row["trace"]["live_decoded_bytes"] == 0
        and row["trace"]["diagnostic_active_intervals"] == 0,
        "Unbalanced trace",
    )


ENVIRONMENT_KEYS = frozenset(
    [
        "PATH",
        "LD_LIBRARY_PATH",
        "LANG",
        "LC_ALL",
        "LC_CTYPE",
        "DISPLAY",
        "XAUTHORITY",
        "XDG_RUNTIME_DIR",
        "FONTCONFIG_FILE",
        "FONTCONFIG_PATH",
    ]
)


def full_cache_inventory(root):
    root = Path(root)
    paths = sorted(root.rglob("*"))
    require(not any(p.is_symlink() for p in paths), "Writable cache contains a symlink")
    return {p.relative_to(root).as_posix(): digest(p) for p in paths if p.is_file()}


def audit(freeze, initial=False):
    if freeze["comparison"] != "P05":
        return base_audit(freeze, initial=initial)
    result = base_audit(freeze, initial=False)
    states = {}
    for arm, item in freeze["initial_caches"].items():
        files = full_cache_inventory(item["root"])
        if initial:
            require(files == item["files"], "Initial complete writable state changed")
        states[arm] = {"file_count": len(files), "files": files}
    result["runtime_cache_states"] = states
    return result


def validate_environment(freeze):
    data = Path(freeze["activation_receipt"]).read_bytes()
    require(
        hashlib.sha256(data).hexdigest() == freeze["activation_receipt_sha256"]
        and freeze["file_bindings"].get(freeze["activation_receipt"])
        == freeze["activation_receipt_sha256"],
        "Activation receipt changed or unbound",
    )
    receipt = json.loads(data)
    require(
        receipt.get("schema") == "freecad-spec003-frozen-pixi-activation/v1"
        and type(receipt.get("exit_code")) is int
        and receipt["exit_code"] == 0,
        "Activation receipt schema/exit invalid",
    )
    require(
        receipt.get("argv") == [freeze["pixi_executable"], "run", "--locked", "/usr/bin/env", "-0"]
        and receipt.get("cwd")
        == freeze["project_root"]
        == str(Path(freeze["pixi_manifest"]).parent),
        "Activation command/project changed",
    )
    require(
        type(receipt.get("stderr_size_bytes")) is int
        and receipt["stderr_size_bytes"] == 0
        and receipt.get("stderr_sha256") == hashlib.sha256(b"").hexdigest(),
        "Selected activation had unexpected stderr",
    )

    for key in [
        "activation_environment",
        "activation_environment_sha256",
        "native_python",
        "native_python_sha256",
        "git_executable",
        "git_sha256",
        "pixi_sha256",
        "pixi_version",
        "manifest_sha256",
        "lock_sha256",
        "env_executable_sha256",
        "capture_script_sha256",
        "native_python_closure",
    ]:
        require(freeze[key] == receipt[key], "Freeze differs from captured activation")
    for name, value in receipt["native_python_closure"].items():
        require(
            freeze["file_bindings"].get(name) == value, "Native Python closure missing or changed"
        )
    for key, path in [
        ("env_executable_sha256", "/usr/bin/env"),
        ("capture_script_sha256", freeze["activation_capture_script"]),
        ("pixi_sha256", freeze["pixi_executable"]),
        ("manifest_sha256", freeze["pixi_manifest"]),
        ("lock_sha256", freeze["pixi_lock"]),
    ]:
        require(
            freeze["file_bindings"].get(path) == receipt[key]
            and digest(Path(path)) == receipt[key],
            "Activation executable/source/manifest identity changed",
        )
    values = freeze["activation_environment"]
    require(
        set(values) <= ENVIRONMENT_KEYS and all(isinstance(v, str) for v in values.values()),
        "Unexpected activation environment variable",
    )
    require("PATH" in values and "DISPLAY" in values, "Missing native environment")
    encoded = json.dumps(values, sort_keys=True, separators=(",", ":")).encode()
    require(
        hashlib.sha256(encoded).hexdigest() == freeze["activation_environment_sha256"],
        "Activation environment changed",
    )
    native = Path(freeze["native_python"]).resolve(strict=True)
    require(
        digest(native) == freeze["native_python_sha256"]
        and freeze["file_bindings"].get(str(native)) == freeze["native_python_sha256"],
        "Native Python identity changed",
    )
    git = Path(shutil.which("git", path=values["PATH"]) or "/nonexistent").resolve(strict=True)
    require(
        str(git) == freeze["git_executable"]
        and digest(git) == freeze["file_bindings"][str(git)] == freeze["git_sha256"],
        "Namespace git identity changed",
    )


def worker_environment(freeze, arm):
    cache = freeze["initial_caches"][arm]["root"]
    # Deliberately build from the frozen allowlist; no inherited operator values.
    env = dict(freeze["activation_environment"])
    env.update(
        PYTHONOPTIMIZE="0",
        PYTHONDONTWRITEBYTECODE="1",
        PYTHONNOUSERSITE="1",
        QT_QPA_PLATFORM="xcb",
        HOME=cache + "/home",
        XDG_CONFIG_HOME=cache + "/config",
        XDG_CACHE_HOME=cache + "/xdg",
        MESA_SHADER_CACHE_DIR=cache + "/mesa",
        FREECAD_USER_HOME=cache + "/home",
        FREECAD_USER_DATA=cache + "/data",
        FREECAD_USER_TEMP=cache + "/temp",
        TMPDIR=cache + "/temp",
    )
    return env


def execution_identity(freeze):
    actual = Path(sys.executable).resolve(strict=True)
    expected = Path(freeze["host_interpreter"]).resolve(strict=True)
    require(
        actual == expected and digest(actual) == freeze["file_bindings"].get(str(expected)),
        "Controller interpreter differs from freeze",
    )
    require(
        callable(getattr(os, "pidfd_open", None))
        and callable(getattr(signal, "pidfd_send_signal", None)),
        "Required pidfd API unavailable",
    )
    owner = OwnedSession()
    owner.start()
    try:
        handle = os.pidfd_open(os.getpid())
        try:
            signal.pidfd_send_signal(handle, 0)
        finally:
            os.close(handle)
    finally:
        owner.close()
    if freeze["comparison"] == "P05":
        validate_environment(freeze)
        for cell in freeze["cells"]:
            for arm in ["baseline", "change"]:
                require(
                    worker_command(freeze, cell, arm) == freeze["worker_commands"][cell][arm],
                    "Qualified worker argv changed",
                )


def load_freeze(path, expected_sha256):
    data = Path(path).read_bytes()
    require(hashlib.sha256(data).hexdigest() == expected_sha256, "Freeze identity changed")
    return json.loads(data)


def preflight(freeze, plan):
    art = Path(freeze["artifact_directory"])
    destinations = [
        art / name
        for name in [
            "status.json",
            "audit-before.json",
            "audit-after.json",
            "observations.jsonl",
            "host.jsonl",
        ]
    ] + [art / f"worker-{i:04d}.bin" for i in range(len(plan))]
    require(
        all(not p.exists() and not p.is_symlink() for p in destinations),
        "Campaign output already exists",
    )
    return audit(freeze, initial=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--freeze", type=Path, required=True)
    parser.add_argument("--freeze-sha256", required=True)
    args = parser.parse_args()
    require(
        sys.flags.isolated == sys.flags.no_site == sys.flags.dont_write_bytecode == 1,
        "Controller requires host Python -I -S -B",
    )
    os.umask(0o077)
    freeze = load_freeze(args.freeze, args.freeze_sha256)
    require(freeze["schema"] == "freecad-spec003-conversion-freeze/v1", "Wrong freeze schema")
    require(digest(Path(__file__)) == freeze["controller_sha256"], "Controller changed")
    require(
        freeze["sampling"]
        == {"pairs": 24, "diagnostic_pairs": 1, "warmup_pairs": 3, "measured_pairs": 20},
        "Sampling policy changed",
    )
    validate_cells(freeze)
    plan = schedule(freeze["comparison"])
    require(set(freeze["cells"]) == {row[0] for row in plan}, "Missing/extra fixture cell")
    art = Path(freeze["artifact_directory"])
    status = art / "status.json"
    execution_identity(freeze)
    before = preflight(freeze, plan)
    write(art / "audit-before.json", before, exclusive=True)
    write(status, {"status": "in_progress", "freeze_sha256": args.freeze_sha256}, exclusive=True)
    completed = 0
    current_cell = None
    cell_start = time.monotonic()
    try:
        with (art / "observations.jsonl").open("x") as output, (art / "host.jsonl").open(
            "x"
        ) as host:
            for ordinal, (cell, pair, arm, phase) in enumerate(plan):
                if cell != current_cell:
                    current_cell = cell
                    cell_start = time.monotonic()
                require(time.monotonic() - cell_start < 1200, "Cell timeout exceeded")
                host.write(json.dumps(host_snapshot()) + "\n")
                host.flush()
                row, raw = run_worker(freeze, cell, arm, art / f"worker-{ordinal:04d}.bin")
                validate_row(
                    row,
                    freeze["cells"][cell]["qualified"][arm],
                    freeze["arms"][arm],
                    freeze["canonical_source"],
                )
                require(time.monotonic() - cell_start < 1200, "Cell timeout exceeded")
                row.update(ordinal=ordinal, cell=cell, pair=pair, arm=arm, phase=phase)
                output.write(json.dumps(row, sort_keys=True) + "\n")
                output.flush()
                completed += 1
                if completed % 16 == 0:
                    print(freeze["comparison"], completed, "/", len(plan), flush=True)
        write(art / "audit-after.json", audit(freeze), exclusive=True)
        write(
            status,
            {
                "status": "complete",
                "completed": completed,
                "failures": 0,
                "retries": 0,
                "freeze_sha256": args.freeze_sha256,
            },
        )
    except BaseException as error:
        write(
            status,
            {
                "status": "failed",
                "completed": completed,
                "error_type": type(error).__name__,
                "freeze_sha256": args.freeze_sha256,
            },
        )
        raise


if __name__ == "__main__":
    main()
