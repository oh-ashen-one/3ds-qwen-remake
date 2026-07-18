#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "demake/core.hpp"

namespace demake {

enum class Bone : std::uint8_t {
    Root,
    Pelvis,
    Torso,
    Head,
    LeftUpperLeg,
    LeftLowerLeg,
    LeftFoot,
    RightUpperLeg,
    RightLowerLeg,
    RightFoot,
    LeftUpperArm,
    LeftLowerArm,
    RightUpperArm,
    RightLowerArm,
    Weapon,
    Count,
};

constexpr std::size_t kRigidBoneCount = static_cast<std::size_t>(Bone::Count);

struct BoneTransform {
    float yaw = 0.0f;
    float forward = 0.0f;
    float vertical = 0.0f;
};

struct RigidPose {
    std::array<BoneTransform, kRigidBoneCount> bones{};

    BoneTransform& at(Bone bone) { return bones[static_cast<std::size_t>(bone)]; }
    const BoneTransform& at(Bone bone) const { return bones[static_cast<std::size_t>(bone)]; }
};

void samplePlayerPose(const Player& player, float elapsed, RigidPose& output);
void sampleBossPose(const Boss& boss, float elapsed, RigidPose& output);

} // namespace demake
