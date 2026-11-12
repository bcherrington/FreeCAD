# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run under an owned FreeCADCmd --safe-mode process; see t001-qualification.md."""


def main():
    import json
    import os
    from pathlib import Path
    import sys
    import unittest

    root = Path(os.environ["SPEC004_ROOT"])
    sys.path.insert(0, str(root / "src/Mod/Sketcher"))
    import FreeCAD as App
    import Sketcher
    from SketcherTests import TestSketcherSolver

    assert "BUILD_PART_DESIGN" in App.__cmake__, "Save/reopen fixture requires PartDesign"
    names = [
        "testRecomputeScalarAndListTransactions",
        "testRecomputeUndoRedoImmediateSolveAndMove",
        "testLegacyExternalGeometryOrientationMigration",
        "testReversedExternalGeometryLeavesSketchInPlace",
        "testSaveLoadWithExternalGeometryReference",
    ]
    names = ["SketcherTests.TestSketcherSolver.TestSketcherSolver." + name for name in names]
    suite = unittest.defaultTestLoader.loadTestsFromNames(names)
    assert suite.countTestCases() == len(names) > 0
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    document = App.newDocument("Spec004PublicSurfaceProbe")
    sketch = document.addObject("Sketcher::SketchObject", "Sketch")
    surface = {name: hasattr(sketch, name) for name in ("addExternal", "attachExternal", "reattachExternal")}
    App.closeDocument(document.Name)
    receipt = {
        "names": names,
        "run": result.testsRun,
        "failures": len(result.failures),
        "errors": len(result.errors),
        "skips": len(result.skipped),
        "FreeCAD": App.Version(),
        "resource_directory": App.getResourceDir(),
        "test_module": TestSketcherSolver.__file__,
        "surface": surface,
    }
    Path("python-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    assert result.wasSuccessful() and result.testsRun == len(names) and not result.skipped
    assert surface == {"addExternal": True, "attachExternal": False, "reattachExternal": False}


main()
