# SPDX-License-Identifier: LGPL-2.1-or-later
"""Standalone Spec 003 T016 check; run with the built FreeCAD Python module.

Set FREECAD_USER_HOME/DATA/TEMP and XDG directories to disposable locations.
Arguments: absolute output directory. The existing source fixture is read only.
"""

import FreeCAD as App
import Part  # noqa: F401 -- register Part's persisted geometry types

# FreeCAD module initialization runs workbench scripts in the main namespace.
# Import the standalone harness dependencies after that initialization.
import hashlib
import json
import math
from pathlib import Path
import sys
import zipfile

FIXTURE_SHA256 = "b173cdc3cf25b50375c4779a459b90484586e42caa69a69c792b313766d627a5"


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def reference_snapshot(document):
    chamfer = document.getObject("Chamfer")
    source, names = chamfer.Base
    assert source.Name == "Revolution"
    assert len(names) == 4
    edges = []
    for name in names:
        edge = source.Shape.getElement(name)
        assert edge.ShapeType == "Edge"
        assert math.isclose(edge.Length, 40.0, abs_tol=1e-7)
        vertices = sorted(tuple(round(v, 7) for v in vertex.Point) for vertex in edge.Vertexes)
        edges.append(vertices)
    # Independent targets from the checksum-pinned legacy model: four top square edges.
    expected = [
        [(-20.0, -20.0, 10.0), (20.0, -20.0, 10.0)],
        [(-20.0, -20.0, 10.0), (-20.0, 20.0, 10.0)],
        [(-20.0, 20.0, 10.0), (20.0, 20.0, 10.0)],
        [(20.0, -20.0, 10.0), (20.0, 20.0, 10.0)],
    ]
    assert sorted(edges) == sorted(expected), (names, edges)
    assert chamfer.Shape.isValid()
    assert not chamfer.Shape.isNull()
    return {
        "object": chamfer.Name,
        "property": "Base",
        "source": source.Name,
        "subelements": list(names),
        "edge_vertices": sorted(edges),
        "chamfer_volume": chamfer.Shape.Volume,
        "chamfer_faces": len(chamfer.Shape.Faces),
    }


def verify_same(before, after):
    for key in ("object", "property", "source", "edge_vertices", "chamfer_faces"):
        assert before[key] == after[key], (key, before[key], after[key])
    assert math.isclose(before["chamfer_volume"], after["chamfer_volume"], rel_tol=1e-10)


def force_recompute(document):
    # The force argument permits recompute; it does not dirty every clean feature.
    for obj in document.Objects:
        if obj.isDerivedFrom("Part::Feature"):
            obj.enforceRecompute()
    count = document.recompute(None, True, True)
    assert count > 0
    return count


def archive_maps(path):
    with zipfile.ZipFile(path) as archive:
        names = [name for name in archive.namelist() if name.endswith(".Map.txt")]
        assert "Revolution.Shape.Map.txt" in names
        assert "Chamfer.Shape.Map.txt" in names
        assert "StringHasher.Table.txt" in archive.namelist()
        return {
            name: {
                "size_bytes": len(archive.read(name)),
                "sha256": hashlib.sha256(archive.read(name)).hexdigest(),
            }
            for name in names
        }


def main():
    root = Path(__file__).resolve().parents[5]
    fixture = Path(__file__).resolve().parent / "TestModels/ModelFromV021.FCStd"
    assert sha256(fixture) == FIXTURE_SHA256
    output = Path(sys.argv[1]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    snapshots = []
    document = None
    generated = []
    try:
        document = App.openDocument(str(fixture))
        snapshots.append(reference_snapshot(document))
        for iteration in range(2):
            force_recompute(document)
            assert "Invalid" not in document.getObject("Chamfer").State
            snapshots.append(reference_snapshot(document))
            verify_same(snapshots[0], snapshots[-1])
            path = output / f"dependent-roundtrip-{iteration + 1}.FCStd"
            document.saveAs(str(path))
            generated.append(
                {
                    "name": path.name,
                    "sha256": sha256(path),
                    "size_bytes": path.stat().st_size,
                    "maps": archive_maps(path),
                }
            )
            App.closeDocument(document.Name)
            document = None
            document = App.openDocument(str(path))
            snapshots.append(reference_snapshot(document))
            verify_same(snapshots[0], snapshots[-1])
        force_recompute(document)
        snapshots.append(reference_snapshot(document))
        verify_same(snapshots[0], snapshots[-1])
    finally:
        if document is not None:
            App.closeDocument(document.Name)
    assert sha256(fixture) == FIXTURE_SHA256
    report = {
        "fixture": {
            "path": str(fixture.relative_to(root)),
            "sha256": FIXTURE_SHA256,
            "size_bytes": fixture.stat().st_size,
        },
        "user_config": App.ConfigGet("UserParameter"),
        "system_config": App.ConfigGet("SystemParameter"),
        "generated": generated,
        "snapshots": snapshots,
        "result": "passed",
        "limits": "One legacy PartDesign model, four Chamfer.Base edges; geometric tolerance 1e-7, volume 1e-10 relative; no byte-order or broader workbench claim.",
    }
    (output / "cad-identity.json").write_text(json.dumps(report, indent=2) + "\n")
    print(
        "PASS: four dependent edge targets across two map-bearing save/reopen cycles and three forced recomputes"
    )


if __name__ == "__main__":
    main()
