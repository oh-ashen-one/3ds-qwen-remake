#!/usr/bin/env python3
"""Validate references, formats, provenance, rigid clips, and per-zone budgets."""

from __future__ import annotations

import json
import hashlib
import sys
from pathlib import Path

from generate_asset_registry import OUTPUT as REGISTRY_OUTPUT
from generate_asset_registry import ZONE_ORDER, generate as generate_registry
from convert_scene_assets import BLOB_OUTPUTS, OUTPUT as SCENE_OUTPUT
from convert_scene_assets import expand_zone, generate as generate_scene, generate_blobs


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "assets" / "manifest.json"
CLIPS = ROOT / "assets" / "animation_clips.json"
BUDGET_REPORT = ROOT / "asset-budget-report.json"
ALLOWED_LICENSES = {"project-original", "public-domain", "cc0"}
KIND_SUFFIXES = {
    "audio_pcm16_mono": {".pcm"},
    "dialogue_text": {".txt"},
    "generated_scene_meshes": {".hpp"},
    "zone_scene_blob": {".bin"},
    "blender_source": {".blend"},
    "texture_atlas": {".t3x"},
    "rigid_animation_clips": {".json"},
    "zone_manifest": {".json"},
}
REQUIRED_CLIPS = {"idle", "locomotion", "light_attack", "heavy_attack", "dodge", "hurt", "heal"}


def fail(message: str) -> None:
    print(f"asset validation failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def inside_root(path: Path) -> bool:
    try:
        path.resolve().relative_to(ROOT.resolve())
        return True
    except ValueError:
        return False


def read_json(path: Path, label: str) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        fail(f"{label} is unreadable JSON: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must be a JSON object")
    return value


def validate_clips() -> int:
    data = read_json(CLIPS, "animation clip manifest")
    bones = data.get("bones", [])
    if data.get("rig_type") != "rigid_hierarchy" or not 12 <= len(bones) <= 16:
        fail("animation rig must be a 12-16 bone rigid hierarchy")
    if data.get("bone_count") != len(bones) or len(set(bones)) != len(bones):
        fail("animation bone_count or bone names are invalid")
    clips = data.get("clips", [])
    clip_ids = {clip.get("id") for clip in clips}
    if clip_ids != REQUIRED_CLIPS:
        fail(f"animation clips must be exactly {sorted(REQUIRED_CLIPS)}")
    for clip in clips:
        duration = float(clip.get("duration_seconds", 0.0))
        rate = int(clip.get("sample_rate", 0))
        keyframes = int(clip.get("keyframes", 0))
        if duration <= 0.0 or rate <= 0 or rate > 30 or keyframes < 2:
            fail(f"clip {clip.get('id')} has an invalid duration/rate/keyframe count")
        expected = int(duration * rate + 0.999)
        if abs(keyframes - expected) > 1:
            fail(f"clip {clip.get('id')} keyframe count does not match duration and rate")
    return len(bones)


def main() -> None:
    data = read_json(MANIFEST, "asset manifest")
    if data.get("schema_version") != 2:
        fail("unsupported asset manifest schema")

    assets = data.get("assets", [])
    if not isinstance(assets, list) or not assets:
        fail("asset list is empty")
    by_id: dict[str, dict] = {}
    for asset in assets:
        if not isinstance(asset, dict):
            fail("asset records must be objects")
        asset_id = asset.get("id", "")
        if not asset_id or asset_id in by_id:
            fail(f"missing or duplicate id: {asset_id!r}")
        by_id[asset_id] = asset
        if asset.get("license") not in ALLOWED_LICENSES:
            fail(f"{asset_id} has disallowed or missing license")
        if not asset.get("provenance"):
            fail(f"{asset_id} has no provenance")
        kind = asset.get("kind")
        if kind not in KIND_SUFFIXES:
            fail(f"{asset_id} has unsupported kind {kind!r}")
        output = ROOT / str(asset.get("output", ""))
        source = ROOT / str(asset.get("source", ""))
        if not inside_root(output) or not inside_root(source):
            fail(f"{asset_id} escapes the repository root")
        if not output.is_file():
            fail(f"{asset_id} output is missing: {output.relative_to(ROOT)}")
        if not source.is_file():
            fail(f"{asset_id} source is missing: {source.relative_to(ROOT)}")
        if output.suffix.lower() not in KIND_SUFFIXES[kind]:
            fail(f"{asset_id} uses incompatible output format {output.suffix}")
        max_bytes = int(asset.get("max_bytes", 0))
        if max_bytes <= 0 or output.stat().st_size > max_bytes:
            fail(f"{asset_id} exceeds or omits its {max_bytes}-byte limit")
        zones = asset.get("zones", [])
        if not zones or any(zone not in ZONE_ORDER for zone in zones):
            fail(f"{asset_id} has invalid or missing zone membership")
        if kind == "audio_pcm16_mono":
            if asset.get("channels") != 1 or asset.get("sample_width_bytes") != 2:
                fail(f"{asset_id} must be PCM16 mono")
            if not 8000 <= int(asset.get("sample_rate", 0)) <= 24000:
                fail(f"{asset_id} sample rate is outside the handheld budget")
            if output.stat().st_size % 2 != 0:
                fail(f"{asset_id} has a truncated PCM16 sample")
        if kind == "texture_atlas":
            if source.suffix.lower() != ".t3s":
                fail(f"{asset_id} must be converted from a tex3ds .t3s descriptor")
            descriptor = source.read_text(encoding="utf-8")
            if "rgb565" not in descriptor or not (source.parent / "environment.ppm").is_file():
                fail(f"{asset_id} must be an RGB565 tex3ds atlas with tracked source pixels")
            texture_data = output.read_bytes()
            if len(texture_data) < 32 or texture_data[:2] != b"\x01\x00":
                fail(f"{asset_id} output has an invalid T3X v1 container header")
        if kind == "blender_source":
            metadata_path = output.with_name(output.name + ".meta.json")
            if not metadata_path.is_file():
                fail(f"{asset_id} is missing its Blender source metadata")
            metadata = read_json(metadata_path, f"{asset_id} metadata")
            scene_source = ROOT / str(metadata.get("scene_source", ""))
            if not scene_source.is_file():
                fail(f"{asset_id} metadata references a missing scene descriptor")
            if metadata.get("scene_source_sha256") != hashlib.sha256(scene_source.read_bytes()).hexdigest():
                fail(f"{asset_id} is stale relative to its scene descriptor")
            if metadata.get("author_script_sha256") != hashlib.sha256(source.read_bytes()).hexdigest():
                fail(f"{asset_id} is stale relative to its Blender authoring script")
            if metadata.get("rig_bone_count") != 15 or metadata.get("static_object_count") != 53:
                fail(f"{asset_id} does not contain the expected 53 objects and 15-bone rig")

    zone_paths = data.get("zone_manifests", {})
    if tuple(zone_paths.keys()) != ZONE_ORDER:
        fail("zone manifests must be ordered interior, vista, arena")
    referenced_assets: set[str] = set()
    zone_budget_rows: list[dict[str, object]] = []
    for zone_id in ZONE_ORDER:
        zone_path = ROOT / str(zone_paths[zone_id])
        if not inside_root(zone_path) or not zone_path.is_file():
            fail(f"{zone_id} zone manifest is missing")
        zone = read_json(zone_path, f"{zone_id} zone manifest")
        if zone.get("schema_version") != 1 or zone.get("id") != zone_id:
            fail(f"{zone_id} zone manifest identity is invalid")
        if int(zone.get("draw_call_budget", 0)) <= 0:
            fail(f"{zone_id} has no draw-call budget")
        if int(zone.get("runtime_budget_bytes", 0)) <= 0:
            fail(f"{zone_id} has no runtime-memory budget")
        romfs_budget = int(zone.get("romfs_budget_bytes", 0))
        if romfs_budget <= 0:
            fail(f"{zone_id} has no RomFS budget")
        zone_assets = zone.get("assets", [])
        if not zone_assets or len(zone_assets) != len(set(zone_assets)):
            fail(f"{zone_id} has missing or duplicate asset references")
        actual_bytes = 0
        for asset_id in zone_assets:
            if asset_id not in by_id:
                fail(f"{zone_id} references unknown asset {asset_id}")
            if zone_id not in by_id[asset_id]["zones"]:
                fail(f"{asset_id} does not declare membership in {zone_id}")
            if by_id[asset_id].get("runtime", True):
                actual_bytes += (ROOT / by_id[asset_id]["output"]).stat().st_size
            referenced_assets.add(asset_id)
        if actual_bytes > romfs_budget:
            fail(f"{zone_id} uses {actual_bytes} bytes over its {romfs_budget}-byte budget")
        zone_budget_rows.append(
            {
                "id": zone_id,
                "display_name": zone.get("display_name"),
                "runtime_budget_bytes": int(zone["runtime_budget_bytes"]),
                "romfs_budget_bytes": romfs_budget,
                "actual_romfs_bytes": actual_bytes,
                "romfs_headroom_bytes": romfs_budget - actual_bytes,
                "draw_call_budget": int(zone["draw_call_budget"]),
                "grid_cell_size": float(zone.get("grid_cell_size", 0.0)),
            }
        )

    required_runtime_assets = {asset_id for asset_id, asset in by_id.items() if asset.get("runtime", True)}
    if not required_runtime_assets.issubset(referenced_assets):
        fail(f"unreferenced runtime assets: {sorted(required_runtime_assets - referenced_assets)}")

    rigid_bone_count = validate_clips()
    expected_registry = generate_registry()
    if not REGISTRY_OUTPUT.is_file() or REGISTRY_OUTPUT.read_text(encoding="utf-8") != expected_registry:
        fail("generated C++ asset registry is stale")
    expected_scene = generate_scene()
    if not SCENE_OUTPUT.is_file() or SCENE_OUTPUT.read_text(encoding="utf-8") != expected_scene:
        fail("generated compact scene data is stale")
    expected_blobs = generate_blobs()
    for zone_id, expected_blob in expected_blobs.items():
        blob_path = BLOB_OUTPUTS[zone_id]
        if not blob_path.is_file() or blob_path.read_bytes() != expected_blob:
            fail(f"generated {zone_id} RomFS scene blob is stale")
    scene_source = read_json(ROOT / "assets" / "scene_source.json", "scene source")
    scene_counts = {zone: len(expand_zone(scene_source["zones"][zone])) for zone in ZONE_ORDER}
    if any(count < 10 for count in scene_counts.values()):
        fail(f"authored zones are unexpectedly sparse: {scene_counts}")

    for row in zone_budget_rows:
        row["static_prop_count"] = scene_counts[str(row["id"])]
    report = {
        "schema_version": 1,
        "project": data.get("project"),
        "asset_count": len(by_id),
        "rigid_bone_count": rigid_bone_count,
        "zones": zone_budget_rows,
        "totals": {
            "static_prop_count": sum(scene_counts.values()),
            "actual_romfs_bytes_across_zone_manifests": sum(
                int(row["actual_romfs_bytes"]) for row in zone_budget_rows
            ),
        },
    }
    BUDGET_REPORT.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    print(
        f"asset validation passed: {len(by_id)} original assets, "
        f"{len(ZONE_ORDER)} authored zones/{sum(scene_counts.values())} static props, "
        f"{rigid_bone_count}-bone rigid clips; budget report {BUDGET_REPORT.name}"
    )


if __name__ == "__main__":
    main()
