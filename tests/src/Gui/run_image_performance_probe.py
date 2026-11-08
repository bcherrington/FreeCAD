#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Launch one fresh, bounded native image probe with isolated settings/addons."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def tree_digest(root):
    digest = hashlib.sha256()
    for path in sorted(root.rglob("*")):
        if path.is_file() and "__pycache__" not in path.parts and ".git" not in path.parts:
            digest.update(path.relative_to(root).as_posix().encode() + b"\0")
            digest.update(hashlib.file_digest(path.open("rb"), "sha256").digest())
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--context", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--gridfinity-addon", type=Path, required=True)
    parser.add_argument("--qualify", action="store_true")
    parser.add_argument("--command-only", action="store_true")
    parser.add_argument("--included-file-probe", action="store_true")
    parser.add_argument("--commit", help="Expected full arm commit from the frozen execution")
    args = parser.parse_args()
    if args.included_file_probe and (not args.command_only or not args.commit):
        parser.error("the included-file probe requires --command-only and an exact --commit")
    if not args.qualify and (not args.commit or len(args.commit) != 40):
        parser.error("acceptance requires the frozen --commit identity")
    context = json.loads(args.context.read_text())
    if tree_digest(args.gridfinity_addon) != context["gridfinity_addon_sha256"]:
        raise ValueError("Gridfinity addon changed")
    with tempfile.TemporaryDirectory(prefix="spec003-image-probe-") as directory:
        work = Path(directory)
        work.chmod(0o700)
        addon = work / "data/Mod/Gridfinity"
        shutil.copytree(
            args.gridfinity_addon, addon, ignore=shutil.ignore_patterns("__pycache__", ".git")
        )
        fixture = work / "fixture.FCStd"
        shutil.copyfile(args.fixture, fixture)
        env = os.environ.copy()
        env.update(
            FREECAD_USER_HOME=str(work / "config"),
            FREECAD_USER_DATA=str(work / "data"),
            FREECAD_USER_TEMP=str(work / "temp"),
            PYTHONDONTWRITEBYTECODE="1",
            PYTHONNOUSERSITE="1",
            QT_QPA_PLATFORM="xcb",
            QT_SCALE_FACTOR="1",
            SPEC003_FIXTURE_PATH=str(fixture),
            SPEC003_SOURCE_ROOT=str(args.source_root.resolve()),
            SPEC003_CONTEXT_PATH=str(args.context.resolve()),
            SPEC003_TRACE_LIBRARY=str(args.build_dir.resolve() / "lib/libFreeCADGui.so"),
        )
        if args.commit:
            env["SPEC003_COMMIT"] = args.commit
        if args.qualify:
            env["SPEC003_QUALIFY"] = "1"
        else:
            env.pop("SPEC003_QUALIFY", None)
        for name in ("QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "LIBGL_ALWAYS_SOFTWARE"):
            env.pop(name, None)
        mod_dirs = [str(p) for p in (args.build_dir / "Mod").iterdir() if p.is_dir()]
        env["LD_LIBRARY_PATH"] = ":".join(
            [str(args.build_dir.resolve() / "lib"), *mod_dirs, env.get("LD_LIBRARY_PATH", "")]
        )
        worker = args.source_root.resolve() / "tests/src/Gui/ImagePerformanceProbe.py"
        if args.included_file_probe:
            worker = args.source_root.resolve() / "tests/src/App/IncludedFilePerformanceProbe.py"
        preamble = (
            "import importlib.util\n"
            + f"_spec = importlib.util.spec_from_file_location('image_probe', {str(worker)!r})\n"
            + "_probe = importlib.util.module_from_spec(_spec)\n_spec.loader.exec_module(_probe)\n"
        )
        if args.command_only:
            macro = work / "probe.py"
            macro.write_text(preamble + "_probe.main_command()\n")
            binary = "FreeCADCmd"
        else:
            macro = work / "probe.FCMacro"
            macro.write_text(
                preamble + "from PySide6 import QtCore\nQtCore.QTimer.singleShot(0, _probe.main)\n"
            )
            binary = "FreeCAD"
        command = [
            str(args.build_dir.resolve() / ("bin/" + binary)),
            "-u",
            str(work / "user.cfg"),
            "-s",
            str(work / "system.cfg"),
            str(macro),
        ]
        result = subprocess.run(command, env=env, stdin=subprocess.DEVNULL, timeout=55)
        record = Path(env["SPEC003_RESULT_FILE"])
        if (
            not record.exists()
            or json.loads(record.read_text()).get("correctness_passed") is not True
        ):
            return 1
        return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
