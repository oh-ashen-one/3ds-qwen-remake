#include "demake/rigid_animation.hpp"

#include <cmath>

namespace demake {
namespace {

float normalizedTimer(float remaining, float duration) {
    if (duration <= 0.0f) {
        return 1.0f;
    }
    const float value = 1.0f - remaining / duration;
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

void sampleLocomotion(float elapsed, RigidPose& pose) {
    const float phase = std::sin(elapsed * 9.0f);
    pose.at(Bone::LeftUpperLeg).forward = phase * 0.20f;
    pose.at(Bone::LeftLowerLeg).forward = -phase * 0.10f;
    pose.at(Bone::LeftFoot).forward = phase * 0.05f;
    pose.at(Bone::RightUpperLeg).forward = -phase * 0.20f;
    pose.at(Bone::RightLowerLeg).forward = phase * 0.10f;
    pose.at(Bone::RightFoot).forward = -phase * 0.05f;
    pose.at(Bone::LeftUpperArm).forward = -phase * 0.11f;
    pose.at(Bone::RightUpperArm).forward = phase * 0.11f;
    pose.at(Bone::Root).vertical = std::fabs(phase) * 0.025f;
}

} // namespace

void samplePlayerPose(const Player& player, float elapsed, RigidPose& output) {
    output = RigidPose{};
    output.at(Bone::Torso).yaw = std::sin(elapsed * 1.7f) * 0.015f;
    output.at(Bone::Head).vertical = std::sin(elapsed * 1.7f) * 0.012f;

    if (player.state == PlayerState::Move) {
        sampleLocomotion(elapsed, output);
    } else if (player.state == PlayerState::Attack) {
        const float swing = sampleRigidSwing(normalizedTimer(player.state_timer, 0.46f));
        output.at(Bone::Torso).yaw = swing * 0.16f;
        output.at(Bone::RightUpperArm).yaw = swing * 0.46f;
        output.at(Bone::RightUpperArm).forward = swing * 0.13f;
        output.at(Bone::RightLowerArm).yaw = swing * 0.32f;
        output.at(Bone::Weapon).yaw = swing;
        output.at(Bone::Weapon).forward = swing * 0.22f;
    } else if (player.state == PlayerState::HeavyAttack) {
        const float swing = sampleRigidSwing(normalizedTimer(player.state_timer, 0.78f)) * 1.2f;
        output.at(Bone::Root).vertical = -0.08f;
        output.at(Bone::Torso).yaw = swing * 0.22f;
        output.at(Bone::RightUpperArm).yaw = swing * 0.55f;
        output.at(Bone::RightUpperArm).forward = swing * 0.18f;
        output.at(Bone::RightLowerArm).yaw = swing * 0.38f;
        output.at(Bone::LeftUpperArm).forward = swing * 0.05f;
        output.at(Bone::Weapon).yaw = swing * 1.1f;
        output.at(Bone::Weapon).forward = swing * 0.28f;
    } else if (player.state == PlayerState::Dodge) {
        const float phase = std::sin(normalizedTimer(player.state_timer, 0.48f) * 3.14159265f);
        output.at(Bone::Root).vertical = -phase * 0.34f;
        output.at(Bone::Torso).forward = phase * 0.24f;
        output.at(Bone::LeftUpperArm).forward = -phase * 0.14f;
        output.at(Bone::RightUpperArm).forward = -phase * 0.14f;
    } else if (player.state == PlayerState::Hurt) {
        const float phase = std::sin(normalizedTimer(player.state_timer, 0.38f) * 3.14159265f);
        output.at(Bone::Torso).yaw = -phase * 0.38f;
        output.at(Bone::Head).forward = -phase * 0.12f;
    } else if (player.state == PlayerState::Heal) {
        const float phase = std::sin(normalizedTimer(player.state_timer, 0.72f) * 3.14159265f);
        output.at(Bone::LeftUpperArm).forward = phase * 0.32f;
        output.at(Bone::RightUpperArm).forward = phase * 0.32f;
        output.at(Bone::LeftLowerArm).yaw = phase * 0.42f;
        output.at(Bone::RightLowerArm).yaw = -phase * 0.42f;
    }
}

void sampleBossPose(const Boss& boss, float elapsed, RigidPose& output) {
    output = RigidPose{};
    output.at(Bone::Torso).yaw = std::sin(elapsed * 2.0f) * 0.02f;
    float swing = 0.0f;
    if (boss.state == BossState::WindupSlash) swing = -0.75f;
    if (boss.state == BossState::Slash) swing = 1.55f;
    if (boss.state == BossState::WindupSlam) swing = -1.2f;
    if (boss.state == BossState::Slam) swing = 0.25f;
    output.at(Bone::RightUpperArm).yaw = swing * 0.52f;
    output.at(Bone::RightUpperArm).forward = swing * 0.15f;
    output.at(Bone::RightLowerArm).yaw = swing * 0.34f;
    output.at(Bone::Weapon).yaw = swing;
    output.at(Bone::Weapon).forward = swing * 0.24f;
    if (boss.state == BossState::Approach) {
        sampleLocomotion(elapsed, output);
    }
}

} // namespace demake
