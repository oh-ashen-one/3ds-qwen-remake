#!/usr/bin/env python3
"""Verify native artifacts, SMDH metadata, RomFS markers, and linked subsystems."""

from __future__ import annotations

import hashlib
import json
import struct
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TARGET = "elden-ring-3ds-demake"
REPORT = ROOT / "build-validation.json"
EXPECTED_TITLE = "Ashen Rift"
EXPECTED_DESCRIPTION = "Original 3DS action-RPG homebrew demo"
EXPECTED_AUTHOR = "oh-ashen-one"


def fail(message: str) -> None:
    print(f"build validation failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def utf16_field(data: bytes, start: int, size: int) -> str:
    raw = data[start : start + size]
    decoded = raw.decode("utf-16-le", errors="strict")
    return decoded.split("\0", 1)[0]


def read_smdh(path: Path) -> dict[str, str]:
    data = path.read_bytes()
    if len(data) < 0x36C0 or data[:4] != b"SMDH":
        fail("SMDH is missing, truncated, or has invalid magic")
    titles = []
    for locale in range(16):
        base = 8 + locale * 0x200
        title = utf16_field(data, base, 0x80)
        description = utf16_field(data, base + 0x80, 0x100)
        author = utf16_field(data, base + 0x180, 0x80)
        if title:
            titles.append({"title": title, "description": description, "author": author})
    expected = {
        "title": EXPECTED_TITLE,
        "description": EXPECTED_DESCRIPTION,
        "author": EXPECTED_AUTHOR,
    }
    if expected not in titles:
        fail(f"SMDH does not contain expected metadata: {expected}")
    return expected


def main() -> None:
    paths = {suffix: ROOT / f"{TARGET}.{suffix}" for suffix in ("3dsx", "elf", "map", "smdh")}
    missing = [path.name for path in paths.values() if not path.is_file() or path.stat().st_size == 0]
    if missing:
        fail(f"missing or empty artifacts: {missing}")

    binary = paths["3dsx"].read_bytes()
    if len(binary) < 44 or binary[:4] != b"3DSX":
        fail("3DSX header is missing or invalid")
    header_size, reloc_header_size = struct.unpack_from("<HH", binary, 4)
    if header_size < 32 or reloc_header_size == 0:
        fail("3DSX header sizes are invalid")
    required_romfs_markers = (b"ambient.pcm", b"keeper.txt", b"romfs:/audio/ambient.pcm")
    missing_markers = [marker.decode("ascii") for marker in required_romfs_markers if marker not in binary]
    if missing_markers:
        fail(f"RomFS or runtime path markers are missing: {missing_markers}")

    smdh = read_smdh(paths["smdh"])
    map_text = paths["map"].read_text(encoding="utf-8", errors="replace")
    required_symbols = (
        "AssetRegistry",
        "GameSession",
        "SceneAssets",
        "samplePlayerPose",
        "AudioStreamer",
        "Tex3DS_TextureImport",
        "CFGU_GetSystemModel",
        "C3D_DrawElements",
    )
    missing_symbols = [symbol for symbol in required_symbols if symbol not in map_text]
    if missing_symbols:
        fail(f"linked subsystem symbols are missing: {missing_symbols}")
    required_embedded_data = ("environment_atlas_t3x",)
    missing_data = [symbol for symbol in required_embedded_data if symbol not in map_text]
    if missing_data:
        fail(f"embedded runtime data is missing: {missing_data}")

    report = {
        "target": TARGET,
        "artifacts": {
            suffix: {"bytes": path.stat().st_size, "sha256": sha256(path)}
            for suffix, path in paths.items()
        },
        "3dsx": {"header_size": header_size, "relocation_header_size": reloc_header_size},
        "smdh": smdh,
        "romfs_markers": [marker.decode("ascii") for marker in required_romfs_markers],
        "linked_subsystems": list(required_symbols),
        "embedded_runtime_data": list(required_embedded_data),
    }
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"build validation passed: {paths['3dsx'].stat().st_size} byte 3DSX, "
        f"SHA-256 {report['artifacts']['3dsx']['sha256']}"
    )


if __name__ == "__main__":
    main()
