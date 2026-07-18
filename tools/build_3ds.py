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
    generated_audio = ROOT / "romfs" / "audio" / "ambient.pcm"
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
    distribution = ROOT / "dist"
    if distribution.is_dir():
        shutil.rmtree(distribution)
    print("cleaned generated build outputs")


def ignored(_: str, names: list[str]) -> set[str]:
    ignored_names = {
        ".git",
        ".DS_Store",
        "build",
        "build-host",
    }
    ignored_names.update(name for name in names if name.startswith(f"{TARGET}.") or name == TARGET)
    return ignored_names


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

    if " " not in str(ROOT):
        subprocess.run(["make", "-f", "Makefile", "-j4"], cwd=ROOT,
                       env=environment, check=True)
        collect_artifacts(ROOT)
    else:
        with tempfile.TemporaryDirectory(prefix="ashen-rift-3ds-") as temporary:
            staging = Path(temporary) / "source"
            shutil.copytree(ROOT, staging, ignore=ignored)
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
