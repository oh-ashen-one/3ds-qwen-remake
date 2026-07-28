#!/usr/bin/env python3
"""Run devkitPro's Make rules from a path that cannot break on spaces."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TARGET = "elden-ring-3ds-demake"
OUTPUT_SUFFIXES = (".3dsx", ".elf", ".lst", ".map", ".smdh")
LOCAL_TOOLCHAIN_ROOT = Path.home() / ".local" / "share" / "elden-ring-3ds-devkit"
GENERATED_BUILD_INPUTS = (
    Path("data/environment_atlas.t3x"),
    Path("include/demake/generated/asset_registry_data.hpp"),
    Path("include/demake/generated/scene_asset_data.hpp"),
    Path("include/demake/generated/scene_assets.stamp"),
    Path("romfs/audio/ashen_deep_hall.pcm"),
    Path("romfs/audio/ashen_gate.pcm"),
    Path("romfs/audio/valley_after_dawn.pcm"),
    Path("romfs/zones/interior.bin"),
    Path("romfs/zones/vista.bin"),
    Path("romfs/zones/arena.bin"),
    Path("romfs/zones/field.bin"),
    Path("romfs/zones/boar_valley.bin"),
    Path("romfs/zones/cloud_plateau.bin"),
)


def clean() -> None:
    for folder in (ROOT / "build", ROOT / "build-host"):
        if folder.is_dir():
            shutil.rmtree(folder)
    for suffix in OUTPUT_SUFFIXES:
        artifact = ROOT / f"{TARGET}{suffix}"
        if artifact.is_file():
            artifact.unlink()
    for report_name in ("asset-budget-report.json", "build-validation.json", "build-report.txt"):
        report = ROOT / report_name
        if report.is_file():
            report.unlink()
    for track_name in ("ashen_deep_hall.pcm", "ashen_gate.pcm", "valley_after_dawn.pcm"):
        generated_audio = ROOT / "romfs" / "audio" / track_name
        if generated_audio.is_file():
            generated_audio.unlink()
    generated_texture = ROOT / "data" / "environment_atlas.t3x"
    if generated_texture.is_file():
        generated_texture.unlink()
    generated_registry = ROOT / "include" / "demake" / "generated" / "asset_registry_data.hpp"
    if generated_registry.is_file():
        generated_registry.unlink()
    generated_scene = ROOT / "include" / "demake" / "generated" / "scene_asset_data.hpp"
    if generated_scene.is_file():
        generated_scene.unlink()
    generated_scene_stamp = ROOT / "include" / "demake" / "generated" / "scene_assets.stamp"
    if generated_scene_stamp.is_file():
        generated_scene_stamp.unlink()
    for zone_id in ("interior", "vista", "arena", "field", "boar_valley", "cloud_plateau"):
        zone_blob = ROOT / "romfs" / "zones" / f"{zone_id}.bin"
        if zone_blob.is_file():
            zone_blob.unlink()
    distribution = ROOT / "dist"
    if distribution.is_dir():
        shutil.rmtree(distribution)
    print("cleaned generated build outputs")


def repository_inventory() -> list[Path]:
    result = subprocess.run(
        [
            "git",
            "-c",
            f"safe.directory={ROOT}",
            "ls-files",
            "-z",
            "--cached",
            "--others",
            "--exclude-standard",
        ],
        cwd=ROOT,
        check=False,
        capture_output=True,
    )
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        raise SystemExit(f"cannot inventory build inputs: {detail or 'unknown git error'}")
    return [Path(os.fsdecode(raw)) for raw in result.stdout.split(b"\0") if raw]


def copy_build_input(relative: Path, staging: Path) -> None:
    if relative.is_absolute() or ".." in relative.parts:
        raise SystemExit(f"unsafe build input path: {relative}")
    source = ROOT / relative
    if not source.is_file():
        raise SystemExit(f"build input is missing or not a file: {relative}")
    destination = staging / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def stage_source_tree(staging: Path) -> None:
    staging.mkdir(parents=True)
    inventory = repository_inventory()
    for relative in inventory:
        copy_build_input(relative, staging)
    for relative in GENERATED_BUILD_INPUTS:
        copy_build_input(relative, staging)


def collect_artifacts(source_root: Path) -> None:
    for suffix in OUTPUT_SUFFIXES:
        artifact_name = f"{TARGET}{suffix}"
        destination = ROOT / artifact_name
        candidates = [source_root / artifact_name, source_root / "build" / artifact_name]
        if suffix in {".map", ".lst"}:
            candidates.extend(source_root.rglob(f"*{suffix}"))
        source = next((candidate for candidate in candidates if candidate.is_file()), None)
        if source is not None and source.resolve() != destination.resolve():
            shutil.copy2(source, destination)


def run_build() -> None:
    environment = os.environ.copy()
    if not environment.get("DEVKITPRO") or not environment.get("DEVKITARM"):
        raise SystemExit("DEVKITPRO and DEVKITARM must be set before building")

    local_toolchain_root = Path(environment.get("ASHEN_3DS_ROOT", LOCAL_TOOLCHAIN_ROOT))
    local_libctru = local_toolchain_root / "libctru"
    if "CTRULIB" not in environment and (local_libctru / "lib" / "libctru.a").is_file():
        environment["CTRULIB"] = str(local_libctru)
    tool_paths = [Path(environment["DEVKITPRO"]) / "tools" / "bin"]
    override = environment.get("ASHEN_3DS_TOOLS")
    if override:
        tool_paths.insert(0, Path(override))
    local_toolchain_bin = local_toolchain_root / "tools" / "bin"
    local_portlibs_bin = local_toolchain_root / "portlibs" / "3ds" / "bin"
    if local_toolchain_bin.is_dir():
        tool_paths.append(local_toolchain_bin)
    if local_portlibs_bin.is_dir():
        tool_paths.append(local_portlibs_bin)
    existing_path = environment.get("PATH", "")
    environment["PATH"] = os.pathsep.join(
        [str(path) for path in tool_paths if path.is_dir()] + [existing_path]
    )
    required_tools = ("picasso", "3dsxtool", "smdhtool")
    missing_tools = [
        tool for tool in required_tools
        if shutil.which(tool, path=environment["PATH"]) is None
    ]
    if missing_tools:
        raise SystemExit(
            "missing official 3DS build tools: " + ", ".join(missing_tools) +
            "; install devkitPro's 3ds-dev group or set ASHEN_3DS_TOOLS"
        )

    with tempfile.TemporaryDirectory(prefix="ashen-rift-3ds-") as temporary:
        staging = Path(temporary) / "source"
        stage_source_tree(staging)
        subprocess.run(["make", "-f", "Makefile", "-j4"], cwd=staging,
                       env=environment, check=True)
        collect_artifacts(staging)

    required = ROOT / f"{TARGET}.3dsx"
    if not required.is_file():
        raise SystemExit(f"cross-build finished without {required.name}")
    print(f"built {required} ({required.stat().st_size} bytes)")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--clean", action="store_true")
    arguments = parser.parse_args()
    if arguments.clean:
        clean()
    else:
        run_build()


if __name__ == "__main__":
    main()
