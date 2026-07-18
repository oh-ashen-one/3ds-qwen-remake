"""Blender authoring view for Ashen Rift's original scene and 15-bone rigid rig.

Run with:
  blender --background --python assets/blender/author_lowpoly.py -- --output /tmp/ashen-rift.blend

The runtime converter reads the same assets/scene_source.json descriptor, so the
Blender source view and generated handheld data cannot silently diverge.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

import bpy


ROOT = Path(__file__).resolve().parents[2]
SCENE_SOURCE = ROOT / "assets" / "scene_source.json"
sys.path.insert(0, str(ROOT / "tools"))
from convert_scene_assets import expand_zone  # noqa: E402

BONES = [
    ("root", None), ("pelvis", "root"), ("torso", "pelvis"), ("head", "torso"),
    ("left_upper_leg", "pelvis"), ("left_lower_leg", "left_upper_leg"),
    ("left_foot", "left_lower_leg"), ("right_upper_leg", "pelvis"),
    ("right_lower_leg", "right_upper_leg"), ("right_foot", "right_lower_leg"),
    ("left_upper_arm", "torso"), ("left_lower_arm", "left_upper_arm"),
    ("right_upper_arm", "torso"), ("right_lower_arm", "right_upper_arm"),
    ("weapon", "right_lower_arm"),
]


def parse_args() -> argparse.Namespace:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(arguments)


def material(name: str, color: list[float]) -> bpy.types.Material:
    value = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    value.diffuse_color = (*color, 1.0)
    return value


def add_box(collection: bpy.types.Collection, box: dict) -> None:
    bpy.ops.mesh.primitive_cube_add(
        size=1.0,
        location=(box["x"], box["y"], box["z"]),
        rotation=(0.0, box["rotation"], 0.0),
    )
    obj = bpy.context.object
    obj.name = box["name"]
    obj.scale = (box["sx"], box["sy"], box["sz"])
    obj.data.materials.append(
        material(f"mat_{box['name']}", [box["red"], box["green"], box["blue"]])
    )
    for owner in list(obj.users_collection):
        owner.objects.unlink(obj)
    collection.objects.link(obj)


def add_rig() -> None:
    armature = bpy.data.armatures.new("AshenRiftRigidRig")
    rig = bpy.data.objects.new("AshenRiftRigidRig", armature)
    bpy.context.scene.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    created = {}
    for index, (name, parent_name) in enumerate(BONES):
        bone = armature.edit_bones.new(name)
        bone.head = (0.0, index * 0.02, index * 0.08)
        bone.tail = (0.0, index * 0.02, index * 0.08 + 0.12)
        if parent_name:
            bone.parent = created[parent_name]
        created[name] = bone
    bpy.ops.object.mode_set(mode="OBJECT")


def main() -> None:
    args = parse_args()
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    source = json.loads(SCENE_SOURCE.read_text(encoding="utf-8"))
    for zone_name, zone in source["zones"].items():
        collection = bpy.data.collections.new(zone_name)
        bpy.context.scene.collection.children.link(collection)
        for box in expand_zone(zone):
            add_box(collection, box)
    add_rig()
    bpy.context.scene["source_descriptor"] = str(SCENE_SOURCE.relative_to(ROOT))
    bpy.context.scene["rig_bone_count"] = len(BONES)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(args.output.resolve()))
    metadata = {
        "schema_version": 1,
        "blender_version": bpy.app.version_string,
        "scene_source": str(SCENE_SOURCE.relative_to(ROOT)),
        "scene_source_sha256": hashlib.sha256(SCENE_SOURCE.read_bytes()).hexdigest(),
        "author_script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "static_object_count": sum(len(expand_zone(zone)) for zone in source["zones"].values()),
        "rig_bone_count": len(BONES),
    }
    metadata_path = args.output.with_name(args.output.name + ".meta.json")
    metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
