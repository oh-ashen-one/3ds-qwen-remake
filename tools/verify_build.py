#!/usr/bin/env python3
"""Verify native artifacts, SMDH metadata, RomFS markers, and linked subsystems."""

from __future__ import annotations

import hashlib
import json
import re
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


def verify_renderer_index_type() -> str:
    renderer = (ROOT / "source" / "renderer.cpp").read_text(encoding="utf-8")
    draw_call = re.search(
        r"C3D_DrawElements\s*\(\s*GPU_TRIANGLES\s*,\s*kCubeIndexCount\s*,\s*"
        r"([A-Z0-9_]+)\s*,\s*index_data_\s*\)",
        renderer,
    )
    if not draw_call:
        fail("indexed cube draw call is missing or no longer matches the validated layout")
    index_type = draw_call.group(1)
    if index_type != "C3D_UNSIGNED_BYTE":
        fail(
            "indexed cube draw must use Citro3D's C3D_UNSIGNED_BYTE; "
            f"found {index_type}"
        )
    return index_type


def verify_asset_budget_report() -> dict[str, object]:
    path = ROOT / "asset-budget-report.json"
    if not path.is_file():
        fail("asset-budget-report.json is missing; run make validate-assets")
    try:
        budget = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        fail(f"asset budget report is unreadable: {error}")
    zones = budget.get("zones", [])
    if [zone.get("id") for zone in zones] != ["interior", "vista", "arena"]:
        fail("asset budget report must contain the three ordered runtime zones")
    for zone in zones:
        actual = int(zone.get("actual_romfs_bytes", -1))
        budget_bytes = int(zone.get("romfs_budget_bytes", -1))
        headroom = int(zone.get("romfs_headroom_bytes", -1))
        if actual < 0 or budget_bytes <= 0 or actual > budget_bytes:
            fail(f"zone {zone.get('id')} exceeds or omits its RomFS budget")
        if headroom != budget_bytes - actual:
            fail(f"zone {zone.get('id')} has inconsistent RomFS headroom")
        if int(zone.get("draw_call_budget", 0)) <= 0:
            fail(f"zone {zone.get('id')} omits its draw-call budget")
    return {
        "path": path.name,
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
        "zones": zones,
    }


def main() -> None:
    renderer_index_type = verify_renderer_index_type()
    asset_budget_report = verify_asset_budget_report()
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
    required_romfs_markers = (
        b"ambient.pcm",
        b"keeper.txt",
        b"romfs:/audio/ambient.pcm",
        b"romfs:/dialogue/keeper.txt",
        b"romfs:/zones/interior.bin",
        b"romfs:/zones/vista.bin",
        b"romfs:/zones/arena.bin",
    )
    missing_markers = [marker.decode("ascii") for marker in required_romfs_markers if marker not in binary]
    if missing_markers:
        fail(f"RomFS or runtime path markers are missing: {missing_markers}")

    smdh = read_smdh(paths["smdh"])
    map_text = paths["map"].read_text(encoding="utf-8", errors="replace")
    required_symbols = (
        "AssetRegistry",
        "GameSession",
        "PlayerController",
        "BossController",
        "ZoneResources",
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
        "renderer_index_type": renderer_index_type,
        "asset_budget_report": asset_budget_report,
    }
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"build validation passed: {paths['3dsx'].stat().st_size} byte 3DSX, "
        f"SHA-256 {report['artifacts']['3dsx']['sha256']}"
    )


if __name__ == "__main__":
    main()
