#!/usr/bin/env python3
"""Deterministic tests for the physical-hardware report validator."""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "validate_hardware_report", ROOT / "tools" / "validate_hardware_report.py"
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("could not load hardware report validator")
VALIDATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VALIDATOR)


def valid_report(artifact_sha256: str) -> dict[str, object]:
    run = {
        "cold_boot": True,
        "duration_seconds": 400,
        "completed": True,
        "average_fps": 30.0,
        "minimum_fps": 24.0,
        "sustained_below_24_fps": False,
        "zone_memory_headroom_percent": {"interior": 25.0, "vista": 26.0, "arena": 24.0},
        "peak_linear_memory_bytes": {"interior": 200000, "vista": 220000, "arena": 240000},
        "audio_underruns": 0,
        "visible_unmasked_loads": 0,
        "crashes": 0,
        "restart_passed": True,
    }
    return {
        "schema_version": 1,
        "tested_at": "2026-07-18T17:00:00-04:00",
        "console": {
            "model": "New 3DS XL",
            "system_version": "test-version",
            "luma3ds_version": "test-version",
            "homebrew_launcher_version": "test-version",
            "new_3ds_mode_confirmed": True,
            "sd_backup_completed": True,
            "sd_health_checked": True,
            "known_homebrew_launch_passed": True,
        },
        "build": {
            "source_commit": "a" * 40,
            "artifact_filename": "elden-ring-3ds-demake.3dsx",
            "artifact_sha256": artifact_sha256,
            "ci_run_url": "https://github.com/oh-ashen-one/elden-ring-3ds-demake/actions/runs/1",
        },
        "deployment": {
            "netload_launch_passed": True,
            "sd_launch_mac_disconnected_passed": True,
            "select_return_passed": True,
            "sleep_wake_passed": True,
        },
        "playthroughs": [{"run": index, **copy.deepcopy(run)} for index in range(1, 4)],
        "edge_cases": {
            "pause_resume_each_zone": True,
            "dialogue_abort_repeat": True,
            "lock_on_loss_boss_death": True,
            "death_during_slash_restart": True,
            "death_during_slam_restart": True,
            "victory_restart": True,
            "zone_transition_cycles": 5,
        },
        "proof": {
            "uncut_video_evidence": "capture-001.mov",
            "physical_console_visible": True,
            "both_screens_visible": True,
            "homebrew_launcher_visible": True,
            "launch_visible": True,
            "gameplay_visible": True,
            "uncut": True,
        },
        "notes": [],
    }


def main() -> None:
    payload = b"physical-build-under-test"
    digest = hashlib.sha256(payload).hexdigest()
    report = valid_report(digest)
    assert VALIDATOR.validate_report(report) == []

    bad_duration = copy.deepcopy(report)
    bad_duration["playthroughs"][1]["duration_seconds"] = 299
    assert any("duration_seconds" in error for error in VALIDATOR.validate_report(bad_duration))

    naive_timestamp = copy.deepcopy(report)
    naive_timestamp["tested_at"] = "2026-07-18T17:00:00"
    assert any("UTC offset" in error for error in VALIDATOR.validate_report(naive_timestamp))

    bad_headroom = copy.deepcopy(report)
    bad_headroom["playthroughs"][2]["zone_memory_headroom_percent"]["arena"] = 19.9
    assert any("arena" in error for error in VALIDATOR.validate_report(bad_headroom))

    emulator_substitute = copy.deepcopy(report)
    emulator_substitute["proof"]["physical_console_visible"] = False
    assert any("physical_console_visible" in error for error in VALIDATOR.validate_report(emulator_substitute))

    with tempfile.TemporaryDirectory(prefix="ashen-rift-hardware-report-") as directory:
        artifact = Path(directory) / "elden-ring-3ds-demake.3dsx"
        artifact.write_bytes(payload)
        assert VALIDATOR.validate_artifact(report, artifact) == []
        artifact.write_bytes(payload + b"changed")
        assert any("mismatch" in error for error in VALIDATOR.validate_artifact(report, artifact))

    print("hardware report validator tests passed")


if __name__ == "__main__":
    main()
