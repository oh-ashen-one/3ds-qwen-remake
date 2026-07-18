#!/usr/bin/env python3
"""Fail builds that violate the original-asset and size-budget contract."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "assets" / "manifest.json"
ALLOWED_LICENSES = {"project-original", "public-domain", "cc0"}
ALLOWED_RUNTIME_SUFFIXES = {".pcm", ".txt", ".cpp"}


def fail(message: str) -> None:
    print(f"asset validation failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if data.get("schema_version") != 1:
        fail("unsupported manifest schema")

    seen: set[str] = set()
    for asset in data.get("assets", []):
        asset_id = asset.get("id", "")
        if not asset_id or asset_id in seen:
            fail(f"missing or duplicate id: {asset_id!r}")
        seen.add(asset_id)
        if asset.get("license") not in ALLOWED_LICENSES:
            fail(f"{asset_id} has disallowed or missing license")
        if not asset.get("provenance"):
            fail(f"{asset_id} has no provenance")
        output = ROOT / asset["output"]
        source = ROOT / asset["source"]
        if not output.is_file():
            fail(f"{asset_id} output is missing: {output.relative_to(ROOT)}")
        if not source.is_file():
            fail(f"{asset_id} source is missing: {source.relative_to(ROOT)}")
        if output.suffix.lower() not in ALLOWED_RUNTIME_SUFFIXES:
            fail(f"{asset_id} uses an unexpected output format: {output.suffix}")
        if output.stat().st_size > int(asset["max_bytes"]):
            fail(f"{asset_id} exceeds {asset['max_bytes']} bytes")

    required_zones = {"interior", "vista", "arena"}
    budgets = data.get("zone_budgets", {})
    if set(budgets) != required_zones:
        fail("zone budgets must contain exactly interior, vista, and arena")
    for zone, budget in budgets.items():
        if int(budget.get("max_draw_calls", 0)) <= 0:
            fail(f"{zone} has no draw-call budget")
        if int(budget.get("max_romfs_bytes", 0)) <= 0:
            fail(f"{zone} has no RomFS budget")

    print(f"asset validation passed: {len(seen)} original assets, {len(budgets)} zone budgets")


if __name__ == "__main__":
    main()
