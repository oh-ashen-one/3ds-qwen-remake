#!/usr/bin/env python3
"""Guard the user-approved CTR-001 runtime and documentation contract."""

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(text: str, needle: str, label: str) -> None:
    assert needle in text, f"{label} is missing {needle!r}"


def float_constant(source: str, name: str) -> float:
    match = re.search(rf"constexpr float {name} = ([0-9.]+)f;", source)
    assert match is not None, f"renderer is missing {name}"
    return float(match.group(1))


def main() -> None:
    main_source = (ROOT / "source" / "main.cpp").read_text(encoding="utf-8")
    input_source = (ROOT / "source" / "game_app.cpp").read_text(encoding="utf-8")
    audio_source = (ROOT / "source" / "audio_streamer.cpp").read_text(encoding="utf-8")
    renderer = (ROOT / "source" / "renderer.cpp").read_text(encoding="utf-8")
    controls = (ROOT / "docs" / "CONTROLS.md").read_text(encoding="utf-8")
    launcher = (ROOT / "GOAL_PROMPT.md").read_text(encoding="utf-8")
    scene = json.loads((ROOT / "assets" / "scene_source.json").read_text(encoding="utf-8"))

    assert "requires New Nintendo 3DS" not in main_source
    assert "return showCompatibilityMessage" not in main_source
    require(input_source, "!new_family_hardware_", "original-family input branch")
    require(input_source, "KEY_DLEFT", "left camera control")
    require(input_source, "KEY_DRIGHT", "right camera control")
    require(input_source, "KEY_DUP", "next-item control")
    require(input_source, "KEY_DDOWN", "previous-item control")
    require(input_source, "KEY_Y", "heavy-attack control")
    require(input_source, "KEY_TOUCH", "touch gameplay controls")
    require(input_source, "hidTouchRead", "touchscreen sampling")
    require(input_source, "input.interact = true", "touch interact control")
    require(input_source, "input.heal = true", "touch heal control")
    require(input_source, "input.lock_toggle = true", "touch lock control")
    require(input_source, "audio_.setZone(world.zone)", "zone-driven music switching")
    require(audio_source, "romfs:/audio/ashen_deep_hall.pcm", "exploration music path")
    require(audio_source, "romfs:/audio/ashen_gate.pcm", "boss music path")
    require(audio_source, "zone == Zone::Arena", "boss-zone music selection")
    require(renderer, "BUILT FOR CTR-001", "title hardware label")
    require(renderer, "objectiveFor", "bottom-screen objective tracker")
    require(renderer, "mapPoint", "bottom-screen live map")
    require(renderer, 'drawTouchButton("ACT"', "bottom-screen action button")
    require(renderer, "distanceToSegment", "cross-zone camera occluder filter")
    require(renderer, "occluder_radius", "generated-prop camera clearance")
    require(renderer, "std::clamp(camera_ground_.x", "interior camera wall constraint")
    require(renderer, "std::min(camera_ground_.z", "interior camera door constraint")
    require(controls, "D-pad Left / Right", "documented camera controls")
    require(controls, "| Y | Heavy attack |", "documented heavy attack")
    require(controls, "Touch ACT", "documented touchscreen gameplay")

    interior_boxes = {box["name"]: box for box in scene["zones"]["interior"]["boxes"]}
    ceiling = interior_boxes["ceiling"]
    ceiling_bottom = float(ceiling["position"][1]) - float(ceiling["scale"][1]) * 0.5
    camera_height = float_constant(renderer, "kInteriorCameraHeight")
    wall_limit = float_constant(renderer, "kInteriorCameraWallLimit")
    front_limit = float_constant(renderer, "kInteriorCameraFrontLimit")
    left_wall = interior_boxes["left_wall"]
    wall_inner_face = abs(float(left_wall["position"][0])) - float(left_wall["scale"][0]) * 0.5

    assert 2.4 < camera_height <= ceiling_bottom - 0.25, (
        "interior camera must stay below the ceiling with clearance"
    )
    assert wall_limit <= wall_inner_face - 0.1, (
        "interior camera orbit must stay inside the wall face"
    )
    assert front_limit <= 4.3, "interior camera must stay behind the closed door face"
    ceiling_zones = [
        zone_name
        for zone_name, zone in scene["zones"].items()
        for box in zone["boxes"]
        if "ceiling" in box["name"]
    ]
    assert ceiling_zones == ["interior"], "only the constrained interior may contain a ceiling"
    assert all(
        not template.get("always", False)
        for zone in scene["zones"].values()
        for generator in zone.get("generators", [])
        for template in generator.get("templates", [generator.get("template", {})])
    ), "generated columns and trees must remain eligible for sightline culling"
    assert len(launcher) < 4000, f"GOAL_PROMPT.md is {len(launcher)} characters"

    print("original 3DS contract tests passed")


if __name__ == "__main__":
    main()
