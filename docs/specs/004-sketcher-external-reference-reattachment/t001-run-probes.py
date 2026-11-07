#!/usr/bin/env python3
"""Compile an owned qualification probe without altering the shared build tree."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import xml.etree.ElementTree as ET


CASES = (
    "legacyCircleRetargetKeepsIdsButAppendsLink",
    "legacyElementObserverSeesMixedState",
    "disableNotifyStillPublishesPropertySignal",
    "setterRestorationLeavesTouchedState",
    "setterRestorationCannotRecoverClearedRedo",
    "legacyDuplicateAdmissionIsNonmutating",
    "legacyReversedLineReencodesOrientation",
    "callerAbortRecoversUnexpectedApplyFailure",
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--build-dir", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    build = (args.build_dir or root / "build/debug").resolve()
    output = args.output_dir or Path(tempfile.mkdtemp(prefix="freecad-spec004-t001-"))
    output = output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / "bin").mkdir(exist_ok=True)
    for name in ("share", "Mod", "lib"):
        (output / name).symlink_to(build / name, target_is_directory=True)
    print(f"Output: {output}", flush=True)
    probe_source = Path(__file__).with_name("t001-native-probe.cpp").resolve()
    files = [
        build / "lib/libFreeCADApp.so",
        build / "lib/libFreeCADBase.so",
        build / "Mod/Sketcher/Sketcher.so",
        build / "Mod/Part/Part.so",
        probe_source,
        Path(__file__).resolve(),
        root / "src/Mod/Sketcher/App/SketchObjectExternal.cpp",
        root / "src/Mod/Sketcher/App/SketchObject.cpp",
        root / "src/Mod/Sketcher/App/SketchObjectConstraints.cpp",
        root / "src/App/Property.cpp",
        root / "src/App/PropertyLinks.cpp",
        root / "src/App/Document.cpp",
    ]
    before = {str(p.relative_to(root)): digest(p) for p in files}
    metadata = json.loads((build / "compile_commands.json").read_text())
    entry = next(
        item for item in metadata
        if item["file"].endswith("/tests/src/Mod/Sketcher/App/SketchObject.cpp")
    )
    command = shlex.split(entry["command"])
    command = command[: command.index("-c")]
    pos = command.index("-o")
    del command[pos : pos + 2]
    ninja = (build / "build.ninja").read_text()
    block = ninja.split("build tests/Sketcher_tests_run:", 1)[1].split("\n\n", 1)[0]
    libraries = next(line for line in block.splitlines() if line.startswith("  LINK_LIBRARIES = "))
    command += [str(probe_source), "-o", str(output / "bin/probe")]
    command += shlex.split(libraries.split(" = ", 1)[1])
    # Current Ninja metadata is authoritative; an obsolete Makefile link.txt may coexist.
    envlib = root / ".pixi/envs/default/lib"
    command += [f"-Wl,-rpath,{envlib}", f"-L{envlib}"]
    (output / "compile-command.json").write_text(json.dumps(command, indent=2) + "\n")

    def run(argv, name, env=None, timeout=120):
        result = subprocess.run(
            argv, cwd=build if name == "compile" else output,
            env=env, capture_output=True, text=True, timeout=timeout,
        )
        (output / f"{name}.log").write_text(result.stdout + result.stderr)
        return result

    compiled = run(command, "compile", timeout=180)
    if compiled.returncode:
        print(compiled.stderr[-5000:])
        raise SystemExit(compiled.returncode)
    env = os.environ.copy()
    module_paths = [build / "lib", *sorted((build / "Mod").glob("*"))]
    env.update(
        FREECAD_USER_HOME=str(output / "user"),
        SPEC004_APP_EXECUTABLE=str(build / "bin/FreeCADCmd"),
        XDG_CONFIG_HOME=str(output / "config"),
        XDG_CACHE_HOME=str(output / "cache"),
        XDG_DATA_HOME=str(output / "data"),
        PYTHONNOUSERSITE="1",
        PYTHONDONTWRITEBYTECODE="1",
        SPEC004_MODULE_PATH=os.pathsep.join(map(str, module_paths)),
    )
    executable = output / "bin/probe"
    discovered = run([str(executable), "--gtest_list_tests"], "discovery", env)
    expected = [f"T001NativeProbe.{name}" for name in CASES]
    discovery_names = [
        f"T001NativeProbe.{line.strip()}" for line in discovered.stdout.splitlines()
        if line.startswith("  ")
    ]
    if discovered.returncode or discovery_names != expected:
        raise SystemExit("Native discovery does not match the exact nonempty manifest")
    result = run(
        [str(executable), "--gtest_filter=T001NativeProbe.*",
         "--gtest_output=xml:" + str(output / "native.xml")],
        "native", env,
    )
    after = {str(p.relative_to(root)): digest(p) for p in files}
    xml = ET.parse(output / "native.xml").getroot()
    executed = [f"{case.attrib['classname']}.{case.attrib['name']}" for case in xml.iter("testcase")]
    receipt = {
        "task": "spec004/T001",
        "purpose": "Observed existing native behavior, not proposed API acceptance",
        "root_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
        "docs_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root / "docs", text=True).strip(),
        "expected": expected, "discovered": discovery_names, "executed": executed,
        "exit_code": result.returncode, "failures": int(xml.attrib["failures"]),
        "inputs_unchanged": before == after, "sha256_before": before, "sha256_after": after,
        "probe_binary_sha256": digest(executable),
        "native_properties": [dict(case.attrib, properties={
            p.attrib["name"]: p.attrib["value"] for p in case.iter("property")
        }) for case in xml.iter("testcase")],
    }
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(result.stdout + result.stderr)
    if result.returncode or executed != expected or before != after:
        raise SystemExit("Probe failed, case coverage incomplete, or shared inputs changed")
    print(f"PASS: exact {len(expected)}-case qualification manifest; source/library hashes stable")


if __name__ == "__main__":
    main()
