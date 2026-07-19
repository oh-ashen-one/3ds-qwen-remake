#!/usr/bin/env python3
"""Guard the user-approved CTR-001 runtime and documentation contract."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(text: str, needle: str, label: str) -> None:
    assert needle in text, f"{label} is missing {needle!r}"


def main() -> None:
    main_source = (ROOT / "source" / "main.cpp").read_text(encoding="utf-8")
    input_source = (ROOT / "source" / "game_app.cpp").read_text(encoding="utf-8")
    renderer = (ROOT / "source" / "renderer.cpp").read_text(encoding="utf-8")
    controls = (ROOT / "docs" / "CONTROLS.md").read_text(encoding="utf-8")
    launcher = (ROOT / "GOAL_PROMPT.md").read_text(encoding="utf-8")

    assert "requires New Nintendo 3DS" not in main_source
    assert "return showCompatibilityMessage" not in main_source
    require(input_source, "!new_family_hardware_", "original-family input branch")
    require(input_source, "KEY_DLEFT", "left camera control")
    require(input_source, "KEY_DRIGHT", "right camera control")
    require(input_source, "KEY_DUP", "next-item control")
    require(input_source, "KEY_DDOWN", "previous-item control")
    require(input_source, "KEY_Y", "heavy-attack control")
    require(input_source, "KEY_TOUCH", "touch diagnostics control")
    require(input_source, "hidTouchRead", "touchscreen sampling")
    require(renderer, "BUILT FOR CTR-001", "title hardware label")
    require(renderer, "D-left/right camera", "bottom-screen controls")
    require(controls, "D-pad Left / Right", "documented camera controls")
    require(controls, "| Y | Heavy attack |", "documented heavy attack")
    assert len(launcher) < 4000, f"GOAL_PROMPT.md is {len(launcher)} characters"

    print("original 3DS contract tests passed")


if __name__ == "__main__":
    main()
