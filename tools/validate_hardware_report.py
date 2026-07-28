#!/usr/bin/env python3
"""Validate measured original Nintendo 3DS acceptance evidence.

This validator deliberately refuses placeholder or emulator-only evidence. It is
not part of the normal CI build gate because a passing report can only be
created while testing the physical console.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "docs" / "HARDWARE_REPORT.json"
ALLOWED_MODELS = {"Nintendo 3DS (CTR-001)"}
ZONES = ("interior", "vista", "arena", "field", "boar_valley", "cloud_plateau")
TRUE_DEPLOYMENT_CHECKS = (
    "netload_launch_passed",
    "sd_launch_mac_disconnected_passed",
    "select_return_passed",
    "sleep_wake_passed",
)
TRUE_EDGE_CHECKS = (
    "pause_resume_each_zone",
    "dialogue_abort_repeat",
    "lock_on_loss_boss_death",
    "death_during_slash_restart",
    "death_during_slam_restart",
    "victory_restart",
    "boar_branch_persistence",
    "cloud_ogre_branch_persistence",
)
TRUE_PROOF_CHECKS = (
    "physical_console_visible",
    "both_screens_visible",
    "homebrew_launcher_visible",
    "launch_visible",
    "gameplay_visible",
    "uncut",
)


def is_number(value: object) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def require_mapping(value: object, path: str, errors: list[str]) -> dict[str, Any]:
    if not isinstance(value, dict):
        errors.append(f"{path} must be an object")
        return {}
    return value


def require_true(mapping: dict[str, Any], keys: tuple[str, ...], path: str, errors: list[str]) -> None:
    for key in keys:
        if mapping.get(key) is not True:
            errors.append(f"{path}.{key} must be true")


def require_nonempty_string(mapping: dict[str, Any], key: str, path: str, errors: list[str]) -> str:
    value = mapping.get(key)
    if not isinstance(value, str) or not value.strip():
        errors.append(f"{path}.{key} must be a non-empty string")
        return ""
    return value.strip()


def validate_report(report: object) -> list[str]:
    errors: list[str] = []
    root = require_mapping(report, "report", errors)
    if root.get("schema_version") != 2:
        errors.append("schema_version must equal 2")

    tested_at = root.get("tested_at")
    if not isinstance(tested_at, str) or not tested_at.strip():
        errors.append("tested_at must be a non-empty ISO-8601 timestamp")
    else:
        try:
            parsed_at = datetime.fromisoformat(tested_at.replace("Z", "+00:00"))
        except ValueError:
            errors.append("tested_at must be a valid ISO-8601 timestamp")
        else:
            if "T" not in tested_at or parsed_at.tzinfo is None:
                errors.append("tested_at must include a time and UTC offset")

    console = require_mapping(root.get("console"), "console", errors)
    model = console.get("model")
    if model not in ALLOWED_MODELS:
        errors.append(f"console.model must be one of {sorted(ALLOWED_MODELS)}")
    if console.get("region") != "Japan":
        errors.append("console.region must equal Japan for the user's target console")
    for key in ("system_version", "luma3ds_version", "homebrew_launcher_version"):
        require_nonempty_string(console, key, "console", errors)
    require_true(
        console,
        ("original_3ds_profile_confirmed", "sd_backup_completed", "sd_health_checked", "known_homebrew_launch_passed"),
        "console",
        errors,
    )

    build = require_mapping(root.get("build"), "build", errors)
    source_commit = require_nonempty_string(build, "source_commit", "build", errors)
    if source_commit and not re.fullmatch(r"[0-9a-fA-F]{40}", source_commit):
        errors.append("build.source_commit must be a full 40-character Git commit")
    artifact_sha256 = require_nonempty_string(build, "artifact_sha256", "build", errors)
    if artifact_sha256 and not re.fullmatch(r"[0-9a-fA-F]{64}", artifact_sha256):
        errors.append("build.artifact_sha256 must be a 64-character SHA-256")
    if build.get("artifact_filename") != "elden-ring-3ds-demake.3dsx":
        errors.append("build.artifact_filename must equal elden-ring-3ds-demake.3dsx")
    ci_run_url = require_nonempty_string(build, "ci_run_url", "build", errors)
    if ci_run_url and not re.fullmatch(
        r"https://github\.com/oh-ashen-one/elden-ring-3ds-demake/actions/runs/[0-9]+", ci_run_url
    ):
        errors.append("build.ci_run_url must reference this repository's GitHub Actions run")

    deployment = require_mapping(root.get("deployment"), "deployment", errors)
    require_true(deployment, TRUE_DEPLOYMENT_CHECKS, "deployment", errors)

    playthroughs = root.get("playthroughs")
    if not isinstance(playthroughs, list) or len(playthroughs) != 3:
        errors.append("playthroughs must contain exactly three runs")
        playthroughs = []
    for index, raw_run in enumerate(playthroughs, start=1):
        path = f"playthroughs[{index - 1}]"
        run = require_mapping(raw_run, path, errors)
        if run.get("run") != index:
            errors.append(f"{path}.run must equal {index}")
        require_true(run, ("cold_boot", "completed", "restart_passed"), path, errors)

        duration = run.get("duration_seconds")
        if not is_number(duration) or not 300 <= duration <= 480:
            errors.append(f"{path}.duration_seconds must be between 300 and 480")
        average_fps = run.get("average_fps")
        if not is_number(average_fps) or average_fps < 30:
            errors.append(f"{path}.average_fps must be at least 30")
        minimum_fps = run.get("minimum_fps")
        if not is_number(minimum_fps) or minimum_fps <= 0:
            errors.append(f"{path}.minimum_fps must be a positive measured value")
        if run.get("sustained_below_24_fps") is not False:
            errors.append(f"{path}.sustained_below_24_fps must be false")
        for key in ("crashes", "audio_underruns", "visible_unmasked_loads"):
            if run.get(key) != 0:
                errors.append(f"{path}.{key} must equal 0")

        headroom = require_mapping(run.get("zone_memory_headroom_percent"), f"{path}.zone_memory_headroom_percent", errors)
        peaks = require_mapping(run.get("peak_linear_memory_bytes"), f"{path}.peak_linear_memory_bytes", errors)
        for zone in ZONES:
            percent = headroom.get(zone)
            if not is_number(percent) or percent < 20:
                errors.append(f"{path}.zone_memory_headroom_percent.{zone} must be at least 20")
            peak = peaks.get(zone)
            if not isinstance(peak, int) or isinstance(peak, bool) or peak <= 0:
                errors.append(f"{path}.peak_linear_memory_bytes.{zone} must be a positive integer")

    edge_cases = require_mapping(root.get("edge_cases"), "edge_cases", errors)
    require_true(edge_cases, TRUE_EDGE_CHECKS, "edge_cases", errors)
    cycles = edge_cases.get("zone_transition_cycles")
    if not isinstance(cycles, int) or isinstance(cycles, bool) or cycles < 5:
        errors.append("edge_cases.zone_transition_cycles must be at least 5")

    proof = require_mapping(root.get("proof"), "proof", errors)
    require_true(proof, TRUE_PROOF_CHECKS, "proof", errors)
    require_nonempty_string(proof, "uncut_video_evidence", "proof", errors)

    notes = root.get("notes")
    if not isinstance(notes, list) or any(not isinstance(note, str) for note in notes):
        errors.append("notes must be an array of strings")
    return errors


def validate_artifact(report: object, artifact: Path) -> list[str]:
    if not artifact.is_file():
        return [f"artifact does not exist: {artifact}"]
    if not isinstance(report, dict) or not isinstance(report.get("build"), dict):
        return ["cannot compare artifact without report.build"]
    expected = report["build"].get("artifact_sha256")
    actual = hashlib.sha256(artifact.read_bytes()).hexdigest()
    if expected != actual:
        return [f"artifact SHA-256 mismatch: report has {expected!r}, measured {actual}"]
    return []


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", nargs="?", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--artifact", type=Path, help="compare the tested report hash to this exact .3dsx")
    args = parser.parse_args()

    report_path = args.report if args.report.is_absolute() else ROOT / args.report
    if not report_path.is_file():
        print(
            f"hardware report validation failed: {report_path} is missing; "
            "copy docs/HARDWARE_REPORT_TEMPLATE.json only when physical testing begins",
            file=sys.stderr,
        )
        raise SystemExit(1)
    try:
        report = json.loads(report_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        print(f"hardware report validation failed: {error}", file=sys.stderr)
        raise SystemExit(1)

    errors = validate_report(report)
    if args.artifact is not None:
        artifact = args.artifact if args.artifact.is_absolute() else ROOT / args.artifact
        errors.extend(validate_artifact(report, artifact))
    if errors:
        for error in errors:
            print(f"hardware report validation failed: {error}", file=sys.stderr)
        raise SystemExit(1)
    print(
        "hardware report passed: physical deployment, three cold boots, "
        "performance, edge cases, and uncut proof are recorded"
    )


if __name__ == "__main__":
    main()
