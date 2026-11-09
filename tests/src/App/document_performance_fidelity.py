# SPDX-License-Identifier: LGPL-2.1-or-later
"""Prospective cross-save oracle; never used to reinterpret previous campaigns."""

from decimal import Decimal
import hashlib
import io
import math
import re
import zipfile

DIRECTION_ABSOLUTE_TOLERANCE = 2e-15
INVARIANT_ABSOLUTE_TOLERANCE = 1e-9
INVARIANT_RELATIVE_TOLERANCE = 1e-12
NUMBER = re.compile(r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?\Z")
SECTIONS = frozenset(
    [
        "Locations",
        "Curve2ds",
        "Curves",
        "Polygon3D",
        "PolygonOnTriangulations",
        "Surfaces",
        "Triangulations",
        "TShapes",
    ]
)


def basis_valid(before, after):
    for frame in (before, after):
        if not all(
            all(math.isfinite(t) for t in row)
            and math.isclose(sum(t * t for t in row), 1.0, rel_tol=0, abs_tol=1e-12)
            for row in frame
        ):
            return False
        if len(frame) == 3:
            if any(
                abs(sum(x * y for x, y in zip(frame[i], frame[j]))) > 1e-12
                for i, j in [(0, 1), (0, 2), (1, 2)]
            ):
                return False
    if len(before) == 3:

        def determinant(v):
            a, b, c = v
            return (
                a[0] * (b[1] * c[2] - b[2] * c[1])
                - a[1] * (b[0] * c[2] - b[2] * c[0])
                + a[2] * (b[0] * c[1] - b[1] * c[0])
            )

        da, db = determinant(before), determinant(after)
        return abs(abs(da) - 1) <= 1e-12 and abs(abs(db) - 1) <= 1e-12 and da * db > 0
    return True


NONFINITE = re.compile(r"(?i)(?<!\S)[+-]?(?:nan|inf|infinity)(?!\S)")
LARGE_NUMBER = re.compile(r"(?<!\S)[+-]?(?:(?:\d+(?:\.\d*)?|\.\d+)[eE]\+?\d{3,}|\d{309,})(?!\S)")
SECTION_PREFIXES = tuple(name + " " for name in SECTIONS)


def equivalent_brep(before, after):
    """Only declared unit-axis slots may drift; structural and integer fields stay exact."""
    for text in (before, after):
        if NONFINITE.search(text) or any(
            not math.isfinite(float(token)) for token in LARGE_NUMBER.findall(text)
        ):
            return False
    left, right = before.splitlines(), after.splitlines()
    if len(left) != len(right) or (
        before and "CASCADE Topology V1, (c) Matra-Datavision" not in left[:3]
    ):
        return False
    section = None
    matrix_rows_left = 0
    matrix_before, matrix_after = [], []
    matrix_changed = False
    for a, b in zip(left, right):
        if a.startswith(SECTION_PREFIXES):
            section = a.split(None, 1)[0]
            matrix_rows_left = 0
        if a == b and section != "Locations":
            continue
        x, y = a.split(), b.split()
        if any(
            t.lower()
            in {"nan", "+nan", "-nan", "inf", "+inf", "-inf", "infinity", "+infinity", "-infinity"}
            or (NUMBER.fullmatch(t) and not math.isfinite(float(t)))
            for t in x + y
        ):
            return False
        if len(x) != len(y):
            return False
        if x and x[0] in SECTIONS:
            section = x[0]
            matrix_rows_left = 0
        groups = []
        if section == "Locations":
            # OCCT location kind 1 is followed by exactly three 3x4 matrix rows.
            if x == ["1"] and matrix_rows_left == 0:
                matrix_rows_left = 3
                matrix_before, matrix_after = [], []
                matrix_changed = False
            elif matrix_rows_left:
                if len(x) != 4 or not all(NUMBER.fullmatch(t) for t in x + y):
                    return False
                groups = [(0, 3)]
                matrix_before.append([float(t) for t in x[:3]])
                matrix_after.append([float(t) for t in y[:3]])
                matrix_rows_left -= 1
        elif section == "Curves" and x:
            if x[0] == "1" and len(x) == 7:
                groups = [(4, 7)]
            elif x[0] == "3" and len(x) == 15:
                groups = [(4, 7), (7, 10), (10, 13)]
        elif section == "Surfaces" and x and x[0] == "1" and len(x) == 13:
            groups = [(4, 7), (7, 10), (10, 13)]
        changed_groups = set()
        for i, (m, n) in enumerate(zip(x, y)):
            if m == n:
                continue
            if not NUMBER.fullmatch(m) or not NUMBER.fullmatch(n):
                return False
            fm, fn = float(m), float(n)
            if not math.isfinite(fm) or not math.isfinite(fn):
                return False
            if Decimal(m) == Decimal(n):
                continue  # Exact decimal identity also handles signed zero.
            group = next((g for g in groups if g[0] <= i < g[1]), None)
            if group is None or abs(fm - fn) > DIRECTION_ABSOLUTE_TOLERANCE:
                return False
            changed_groups.add(group)
        if section == "Locations":
            matrix_changed = matrix_changed or bool(changed_groups)
            if (
                matrix_rows_left == 0
                and matrix_changed
                and not basis_valid(matrix_before, matrix_after)
            ):
                return False
        if changed_groups and len(groups) == 3:
            try:
                if not basis_valid(
                    [[float(t) for t in x[lo:hi]] for lo, hi in groups],
                    [[float(t) for t in y[lo:hi]] for lo, hi in groups],
                ):
                    return False
            except ValueError:
                return False
        for lo, hi in changed_groups:
            try:
                values = [[float(t) for t in tokens[lo:hi]] for tokens in (x, y)]
            except ValueError:
                return False
            if not all(
                all(math.isfinite(t) for t in v)
                and math.isclose(sum(t * t for t in v), 1.0, rel_tol=0, abs_tol=1e-12)
                for v in values
            ):
                return False
    return matrix_rows_left == 0


def shape_invariants(shape):
    result = {"null": shape.isNull()}
    if result["null"]:
        return result
    result["type"] = shape.ShapeType
    result["valid"] = shape.isValid()
    result["topology"] = {
        name: len(getattr(shape, name))
        for name in [
            "Vertexes",
            "Edges",
            "Wires",
            "Faces",
            "Shells",
            "Solids",
            "CompSolids",
            "Compounds",
        ]
    }
    box = shape.BoundBox
    result["geometry"] = {
        "bbox_mm": [getattr(box, key) for key in ["XMin", "YMin", "ZMin", "XMax", "YMax", "ZMax"]],
        "length_mm": [shape.Length],
        "area_mm2": [shape.Area],
        "volume_mm3": [shape.Volume],
    }
    result["center_applicable"] = shape.ShapeType in {"Edge", "Wire", "Face", "Shell", "Solid"}
    if result["center_applicable"]:
        result["geometry"]["center_mm"] = list(shape.CenterOfMass)
    return result


def equivalent_invariants(before, after):
    a, b = dict(before), dict(after)
    ag, bg = a.pop("geometry", {}), b.pop("geometry", {})
    return (
        a == b
        and ag.keys() == bg.keys()
        and all(
            len(ag[key]) == len(bg[key])
            and all(
                math.isfinite(x)
                and math.isfinite(y)
                and math.isclose(
                    x, y, rel_tol=INVARIANT_RELATIVE_TOLERANCE, abs_tol=INVARIANT_ABSOLUTE_TOLERANCE
                )
                for x, y in zip(ag[key], bg[key])
            )
            for key in ag
        )
    )


def persistent_state(doc):
    """All persistent object property schemas/serialized values; geometry/payloads separate."""
    rows = []
    property_count = member_count = total_bytes = 0
    for obj in sorted(doc.Objects, key=lambda item: item.Name):
        properties = []
        for name in sorted(obj.PropertiesList):
            flags = set(obj.getTypeOfProperty(name)) | set(obj.getPropertyStatus(name))
            if flags & {"Transient", "NoPersist", "PropTransient", "PropNoPersist"}:
                continue
            property_count += 1
            if property_count > 100000:
                raise RuntimeError("Excessive persistent property count")
            kind = obj.getTypeIdOfProperty(name)
            row = {
                "name": name,
                "type": kind,
                "type_flags": sorted(obj.getTypeOfProperty(name)),
                "status": sorted(obj.getPropertyStatus(name), key=str),
            }
            if kind == "Part::PropertyPartShape":
                row["separate_oracle"] = "geometry"
            elif kind == "App::PropertyFileIncluded":
                row["separate_oracle"] = "included-bytes-and-declaration"
            elif kind == "App::PropertyPythonObject" and name == "Proxy":
                row["separate_oracle"] = "proxy-class-identity"
            else:
                raw = bytes(obj.dumpPropertyContent(name, Compression=0))
                if len(raw) > 128 * 1024 * 1024:
                    raise RuntimeError("Oversized persistent property")
                with zipfile.ZipFile(io.BytesIO(raw)) as archive:
                    entries = archive.infolist()
                    if (
                        len(entries) > 128
                        or len({e.filename for e in entries}) != len(entries)
                        or sum(e.file_size for e in entries) > 128 * 1024 * 1024
                    ):
                        raise RuntimeError("Unbounded persistent property archive")
                    member_count += len(entries)
                    total_bytes += sum(entry.file_size for entry in entries)
                    if member_count > 100000 or total_bytes > 1024 * 1024 * 1024:
                        raise RuntimeError("Excessive aggregate persistent state")
                    row["members"] = [
                        [entry.filename, hashlib.sha256(archive.read(entry)).hexdigest()]
                        for entry in sorted(entries, key=lambda entry: entry.filename)
                    ]
            properties.append(row)
        rows.append({"name": obj.Name, "type": obj.TypeId, "properties": properties})
    return {
        "objects": rows,
        "inventory": {
            "properties": property_count,
            "members": member_count,
            "decompressed_bytes": total_bytes,
        },
    }


def geometric_state(doc, model):
    """Retain in-memory shape bytes only; publication emits opaque identities/counts."""
    shapes = {}
    for obj in sorted(doc.Objects, key=lambda item: item.Name):
        for name in sorted(obj.PropertiesList):
            if obj.getTypeIdOfProperty(name) == "Part::PropertyPartShape":
                shape = getattr(obj, name)
                invariants = shape_invariants(shape)
                shapes[(obj.Name, name)] = (
                    "" if invariants["null"] else shape.exportBrepToString(),
                    invariants,
                )
    for row in model:
        for prop, target, references in row["references"]:
            for reference in references:
                if len(reference) == 4 and isinstance(reference[3], str):
                    shape = doc.getObject(target).getSubObject(reference[0])
                    shapes[(row["name"], prop, target, reference[0])] = (
                        shape.exportBrepToString(),
                        shape_invariants(shape),
                    )
    return shapes


def equivalent_geometry(before, after):
    return before.keys() == after.keys() and all(
        equivalent_brep(before[key][0], after[key][0])
        and equivalent_invariants(before[key][1], after[key][1])
        for key in before
    )


def without_geometry_hashes(model):
    """Remove only hashes independently covered by the complete geometry oracle."""
    import copy

    result = copy.deepcopy(model)
    for row in result:
        row.pop("shape_sha256", None)
        for _, _, references in row["references"]:
            for reference in references:
                if len(reference) == 4 and isinstance(reference[3], str):
                    reference[3] = "separate-geometry-oracle"
    return result


def geometry_manifest(doc, model):
    """Exact hashes for all PartShape properties and checked referenced subshapes."""
    shapes = {}
    for obj in sorted(doc.Objects, key=lambda item: item.Name):
        for name in sorted(obj.PropertiesList):
            if obj.getTypeIdOfProperty(name) == "Part::PropertyPartShape":
                shape = getattr(obj, name)
                shapes[(obj.Name, name)] = (
                    None
                    if shape.isNull()
                    else hashlib.sha256(shape.exportBrepToString().encode()).hexdigest()
                )
    for row in model:
        for prop, target, references in row["references"]:
            for reference in references:
                if len(reference) == 4 and isinstance(reference[3], str):
                    shape = doc.getObject(target).getSubObject(reference[0])
                    shapes[(row["name"], prop, target, reference[0])] = (
                        None
                        if shape.isNull()
                        else hashlib.sha256(shape.exportBrepToString().encode()).hexdigest()
                    )
    return [[list(key), value] for key, value in sorted(shapes.items())]
