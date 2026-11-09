# SPDX-License-Identifier: LGPL-2.1-or-later
"""Preserve public generated fixture bytes durably, with explicit public metadata.

Input directories are owned preparation outputs. Geometry members are copied
unchanged; only FCStd license/creator metadata and a STEP license comment change.
This is not a license change for imported data: inputs must be our numeric fixtures.
"""
from io import BytesIO
from pathlib import Path, PurePosixPath
import getpass
import hashlib
import json
import re
import sys
import zipfile


def digest(data):
    return hashlib.sha256(data).hexdigest()


def freeze():
    root = Path.cwd()
    package = root / "docs/specs/005-mainstream-cad-performance-investigation"
    private = root / "build/spec005-t001"
    preparations = [
        private / "fixtures-final/preparation.json",
        private / "auxiliary-canonical-source/preparation.json",
    ]
    rows = [row for path in preparations for row in json.loads(path.read_text())]
    assert len(rows) == 17
    destination = package / "fixtures/canonical"
    destination.mkdir(exist_ok=True)
    records = []
    for row in rows:
        assert re.fullmatch(r"(?:W0[1-6]|Q)-[A-Za-z0-9-]+", row["id"])
        for source_record in [row] + ([row["step"]] if "step" in row else []):
            source = root / source_record["path"]
            assert source.resolve().is_relative_to(private.resolve()) and not source.is_symlink()
            data = source.read_bytes()
            assert digest(data) == source_record["sha256"]
            if source.suffix == ".FCStd":
                stream = BytesIO()
                with zipfile.ZipFile(BytesIO(data)) as old, zipfile.ZipFile(stream, "w") as new:
                    for info in old.infolist():
                        assert not PurePosixPath(info.filename).is_absolute()
                        assert ".." not in PurePosixPath(info.filename).parts
                        member = old.read(info.filename)
                        if info.filename == "Document.xml":
                            for key, value in [
                                ("License", "LGPL-2.1-or-later"),
                                ("CreatedBy", "FreeCAD Spec005 fixture generator"),
                            ]:
                                pattern = (
                                    rb'(<Property name="'
                                    + key.encode()
                                    + rb'"[^>]*>\s*<String value=")[^"]*("/>)'
                                )
                                member, count = re.subn(
                                    pattern,
                                    lambda match: match.group(1) + value.encode() + match.group(2),
                                    member,
                                    count=1,
                                )
                                assert count == 1, key
                        new.writestr(info, member)
                frozen = stream.getvalue()
            else:
                assert source.suffix == ".step"
                frozen = data.replace(
                    b"HEADER;",
                    b"HEADER;\n/* Public generated Spec005 analytic fixture; SPDX-License-Identifier: LGPL-2.1-or-later. */",
                    1,
                )
            assert b"/home/" not in frozen and getpass.getuser().encode() not in frozen
            target = destination / source.name
            if target.exists():
                assert target.read_bytes() == frozen, "Never replace a frozen canonical identity"
            else:
                target.write_bytes(frozen)
            target.chmod(0o444)
            # Geometry data are byte-identical: license text cannot redefine a shape.
            if source.suffix == ".FCStd":
                with zipfile.ZipFile(BytesIO(data)) as old, zipfile.ZipFile(BytesIO(frozen)) as new:
                    assert old.namelist() == new.namelist()
                    for name in old.namelist():
                        assert b"/home/" not in new.read(name)
                        assert getpass.getuser().encode() not in new.read(name)
                        if name != "Document.xml":
                            assert old.read(name) == new.read(name), name
            records.append(
                {
                    "id": row["id"],
                    "source_path": str(source.relative_to(root)),
                    "source_sha256": digest(data),
                    "canonical_path": str(target.relative_to(root)),
                    "canonical_sha256": digest(frozen),
                    "bytes": len(frozen),
                    "normalization": "public creator/license only; shape members unchanged",
                }
            )
    report = {
        "phase": "T001 durable public fixture preservation, not timing evidence",
        "helper_sha256": digest(Path(__file__).read_bytes()),
        "files": records,
    }
    (private / "canonical-index.json").write_text(json.dumps(report, indent=2) + "\n")
    print(
        "Preserved", len(records), "public canonical files; bytes", sum(x["bytes"] for x in records)
    )


if __name__ == "__main__":
    freeze()
