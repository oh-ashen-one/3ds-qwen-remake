#!/usr/bin/env python3
"""Create and verify the persistent Homebrew Launcher SD-card bundle."""

from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TARGET = "elden-ring-3ds-demake"
DIST = ROOT / "dist"
SD_RELATIVE = Path("3ds") / TARGET / f"{TARGET}.3dsx"
INFO_RELATIVE = Path("3ds") / TARGET / "build-info.json"


def fail(message: str) -> None:
    raise SystemExit(f"SD package validation failed: {message}")


def sha256_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def source_commit() -> str:
    git = ["git", "-c", f"safe.directory={ROOT}"]
    result = subprocess.run(
        [*git, "rev-parse", "HEAD"],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        fail("cannot resolve the source commit")
    status = subprocess.run(
        [*git, "status", "--porcelain", "--untracked-files=no"],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if status.returncode != 0:
        fail("cannot verify whether the source worktree is clean")
    if status.stdout.strip():
        fail("refusing to create a revision-labeled SD bundle from a dirty worktree")
    return result.stdout.strip()


def add_deterministic(zip_file: zipfile.ZipFile, relative: Path, payload: bytes) -> None:
    entry = zipfile.ZipInfo(relative.as_posix(), date_time=(1980, 1, 1, 0, 0, 0))
    entry.compress_type = zipfile.ZIP_DEFLATED
    entry.external_attr = 0o100644 << 16
    zip_file.writestr(entry, payload)


def main() -> None:
    artifact = ROOT / f"{TARGET}.3dsx"
    validation_path = ROOT / "build-validation.json"
    if not artifact.is_file() or not validation_path.is_file():
        fail("run make verify-build before packaging")

    validation = json.loads(validation_path.read_text(encoding="utf-8"))
    expected = validation.get("artifacts", {}).get("3dsx", {})
    payload = artifact.read_bytes()
    artifact_hash = sha256_bytes(payload)
    if expected.get("bytes") != len(payload) or expected.get("sha256") != artifact_hash:
        fail("3DSX does not match build-validation.json")

    if DIST.exists():
        if DIST.resolve().parent != ROOT.resolve():
            fail("refusing to replace a distribution directory outside the repository")
        shutil.rmtree(DIST)

    bundle_root = DIST / "sdmc" / SD_RELATIVE.parent
    bundle_root.mkdir(parents=True)
    shutil.copy2(artifact, bundle_root / artifact.name)

    build_info = {
        "schema_version": 1,
        "project": "Ashen Rift",
        "source_commit": source_commit(),
        "install_path": f"sdmc:/{SD_RELATIVE.as_posix()}",
        "artifact": {
            "name": artifact.name,
            "bytes": len(payload),
            "sha256": artifact_hash,
        },
    }
    build_info_payload = (json.dumps(build_info, indent=2, sort_keys=True) + "\n").encode()
    (bundle_root / "build-info.json").write_bytes(build_info_payload)

    archive = DIST / "ashen-rift-sd-bundle.zip"
    with zipfile.ZipFile(archive, "w") as zip_file:
        add_deterministic(zip_file, SD_RELATIVE, payload)
        add_deterministic(zip_file, INFO_RELATIVE, build_info_payload)

    with zipfile.ZipFile(archive) as zip_file:
        names = zip_file.namelist()
        if names != [SD_RELATIVE.as_posix(), INFO_RELATIVE.as_posix()]:
            fail(f"unexpected archive entries: {names}")
        if sha256_bytes(zip_file.read(SD_RELATIVE.as_posix())) != artifact_hash:
            fail("archived 3DSX hash does not match the verified build")
        archived_info = json.loads(zip_file.read(INFO_RELATIVE.as_posix()))
        if archived_info != build_info:
            fail("archived build metadata is inconsistent")

    archive_payload = archive.read_bytes()
    package_validation = {
        "schema_version": 1,
        "archive": {
            "name": archive.name,
            "bytes": len(archive_payload),
            "sha256": sha256_bytes(archive_payload),
        },
        "install_path": build_info["install_path"],
        "source_commit": build_info["source_commit"],
        "artifact": build_info["artifact"],
        "entries": [SD_RELATIVE.as_posix(), INFO_RELATIVE.as_posix()],
    }
    (DIST / "sd-bundle-validation.json").write_text(
        json.dumps(package_validation, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(
        f"SD package validation passed: {archive} ({len(archive_payload)} bytes), "
        f"3DSX SHA-256 {artifact_hash}"
    )


if __name__ == "__main__":
    main()
