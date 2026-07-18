#include "demake/core.hpp"
#include "demake/asset_registry.hpp"
#include "demake/rigid_animation.hpp"
#include "demake/scene_assets.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

using namespace demake;

namespace {

void stepMany(GameSimulation& game, const InputFrame& input, int count) {
    for (int i = 0; i < count; ++i) {
        game.step(input, kFixedStep);
    }
}

void testMathAndCollision() {
    assert(std::fabs(length({3.0f, 4.0f}) - 5.0f) < 0.001f);
    assert(circlesOverlap({0.0f, 0.0f}, 1.0f, {1.5f, 0.0f}, 0.6f));
    assert(!circlesOverlap({0.0f, 0.0f}, 1.0f, {2.0f, 0.0f}, 0.6f));
    assert(std::fabs(sampleRigidSwing(0.0f)) < 0.001f);
    assert(sampleRigidSwing(0.5f) > 1.4f);
    assert(std::fabs(sampleRigidSwing(1.0f)) < 0.001f);
}

void testGeneratedAssetRegistry() {
    assert(AssetRegistry::assetCount() == 9);
    const AssetRecord* ambient = AssetRegistry::find("ambient_sable_expanse");
    assert(ambient != nullptr);
    assert(AssetRegistry::assetBelongsToZone(*ambient, Zone::Interior));
    assert(AssetRegistry::assetBelongsToZone(*ambient, Zone::Vista));
    assert(AssetRegistry::assetBelongsToZone(*ambient, Zone::Arena));
    const AssetRecord* dialogue = AssetRegistry::find("veiled_keeper_dialogue");
    assert(dialogue != nullptr);
    assert(!AssetRegistry::assetBelongsToZone(*dialogue, Zone::Interior));
    assert(AssetRegistry::assetBelongsToZone(*dialogue, Zone::Vista));
    const AssetRecord* texture = AssetRegistry::find("environment_texture_atlas");
    assert(texture != nullptr);
    assert(std::strcmp(texture->kind, "texture_atlas") == 0);
    assert(AssetRegistry::assetBelongsToZone(*texture, Zone::Arena));
    assert(AssetRegistry::zone(Zone::Arena).draw_call_budget == 84);
    assert(AssetRegistry::find("missing") == nullptr);
}

void testGeneratedSceneData() {
    assert(SceneAssets::boxCount(Zone::Interior) == 16);
    assert(SceneAssets::boxCount(Zone::Vista) == 24);
    assert(SceneAssets::boxCount(Zone::Arena) == 13);
    std::size_t count = 0;
    const SceneBox* arena = SceneAssets::boxes(Zone::Arena, count);
    assert(arena != nullptr && count == 13);
    assert(arena[0].sx > 0.0f && arena[0].sy > 0.0f && arena[0].sz > 0.0f);
}

void testRigidPoseSampling() {
    static_assert(kRigidBoneCount == 15, "the runtime rig must stay inside the 12-16 bone budget");
    Player player{};
    player.state = PlayerState::Attack;
    player.state_timer = 0.23f;
    RigidPose attack{};
    samplePlayerPose(player, 1.0f, attack);
    assert(attack.at(Bone::Weapon).yaw > 1.3f);
    assert(attack.at(Bone::RightUpperArm).forward > 0.1f);

    player.state = PlayerState::Move;
    RigidPose locomotion{};
    samplePlayerPose(player, 0.2f, locomotion);
    assert(locomotion.at(Bone::LeftUpperLeg).forward *
               locomotion.at(Bone::RightUpperLeg).forward < 0.0f);
}

void testMovementAndStamina() {
    GameSimulation game;
    InputFrame move{};
    move.move_z = 1.0f;
    move.sprint_held = true;
    stepMany(game, move, 20);
    assert(game.world().player.position.z > -4.0f);
    assert(game.world().player.stamina < 100.0f);
}

void testQuickItemSelection() {
    GameSimulation game;
    InputFrame next{};
    next.item_delta = 1;
    game.step(next, kFixedStep);
    assert(game.world().player.selected_item == 1);
    assert(std::strcmp(quickItemName(game.world().player.selected_item), "Warden Effigy") == 0);

    InputFrame previous{};
    previous.item_delta = -1;
    game.step(previous, kFixedStep);
    assert(game.world().player.selected_item == 0);
    game.step(previous, kFixedStep);
    assert(game.world().player.selected_item == 2);
}

void testZoneHandoffs() {
    GameSimulation game;
    assert(ZoneManager::isLoaded(game.world(), Zone::Interior));
    assert(!ZoneManager::isLoaded(game.world(), Zone::Vista));
    assert(game.world().zone_loads == 1);
    assert(game.world().zone_resident_bytes ==
           AssetRegistry::zone(Zone::Interior).runtime_budget_bytes);
    game.mutableWorld().player.position = {0.0f, 3.5f};
    InputFrame interact{};
    interact.interact = true;
    game.step(interact, kFixedStep);
    assert(ZoneManager::isLoaded(game.world(), Zone::Interior));
    assert(ZoneManager::isLoaded(game.world(), Zone::Vista));
    assert(game.world().zone_loads == 2);
    stepMany(game, InputFrame{}, 55);
    game.mutableWorld().player.position.z = 5.4f;
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().zone == Zone::Vista);
    assert(!ZoneManager::isLoaded(game.world(), Zone::Interior));
    assert(ZoneManager::isLoaded(game.world(), Zone::Vista));
    assert(game.world().zone_unloads == 1);
    assert(game.world().zone_transitions == 1);
    assert(game.world().zone_resident_bytes ==
           AssetRegistry::zone(Zone::Vista).runtime_budget_bytes);

    game.mutableWorld().player.position = {0.0f, 15.5f};
    game.step(interact, kFixedStep);
    game.step(InputFrame{}, kFixedStep);
    game.step(interact, kFixedStep);
    assert(game.world().dialogue_complete);
    game.mutableWorld().player.position = {0.0f, 27.5f};
    game.step(interact, kFixedStep);
    assert(ZoneManager::isLoaded(game.world(), Zone::Arena));
    stepMany(game, InputFrame{}, 30);
    assert(game.world().zone == Zone::Arena);
    assert(!ZoneManager::isLoaded(game.world(), Zone::Vista));
    assert(ZoneManager::isLoaded(game.world(), Zone::Arena));
    assert(game.world().zone_unloads == 2);
    assert(game.world().zone_transitions == 2);
}

void testDialogueAbortAndRetry() {
    GameSimulation game;
    WorldState& world = game.mutableWorld();
    world.zone = Zone::Vista;
    world.player.position = {0.0f, 15.5f};
    InputFrame interact{};
    interact.interact = true;
    game.step(interact, kFixedStep);
    assert(game.world().dialogue_active);

    InputFrame cancel{};
    cancel.dodge_pressed = true;
    game.step(cancel, kFixedStep);
    assert(!game.world().dialogue_active);
    assert(!game.world().dialogue_complete);

    game.step(interact, kFixedStep);
    assert(game.world().dialogue_active);
    game.step(InputFrame{}, kFixedStep);
    game.step(interact, kFixedStep);
    assert(game.world().dialogue_complete);
}

void testSessionLifecycleAndPause() {
    GameSession session;
    assert(session.mode() == SessionMode::Title);
    InputFrame confirm{};
    confirm.interact = true;
    session.step(confirm, kFixedStep);
    assert(session.mode() == SessionMode::Playing);

    const Vec2 before = session.simulation().world().player.position;
    InputFrame pause{};
    pause.pause_toggle = true;
    session.step(pause, kFixedStep);
    assert(session.mode() == SessionMode::Paused);
    InputFrame move{};
    move.move_z = 1.0f;
    session.step(move, kFixedStep);
    assert(distance(before, session.simulation().world().player.position) < 0.001f);

    session.suspend();
    assert(session.mode() == SessionMode::Suspended);
    session.resume();
    assert(session.mode() == SessionMode::Paused);
    session.step(pause, kFixedStep);
    assert(session.mode() == SessionMode::Playing);
    session.step(move, kFixedStep);
    assert(distance(before, session.simulation().world().player.position) > 0.01f);
}

void testCombatAndLockOn() {
    GameSimulation game;
    WorldState& world = game.mutableWorld();
    world.zone = Zone::Arena;
    world.player.position = {0.0f, 2.8f};
    world.boss.position = {0.0f, 5.0f};
    world.boss.state = BossState::Recover;
    world.boss.state_timer = 5.0f;

    InputFrame lock{};
    lock.lock_toggle = true;
    game.step(lock, kFixedStep);
    assert(game.world().player.lock_on);

    const float health_before = game.world().boss.health;
    InputFrame attack{};
    attack.light_attack = true;
    game.step(attack, kFixedStep);
    stepMany(game, InputFrame{}, 15);
    assert(game.world().boss.health < health_before);
    assert(game.world().player.stamina < 100.0f);
}

void testLockOnTargetLoss() {
    GameSimulation game;
    WorldState& world = game.mutableWorld();
    world.zone = Zone::Arena;
    world.player.position = {-8.5f, -8.5f};
    world.player.lock_on = true;
    world.boss.position = {8.5f, 8.5f};
    world.boss.state = BossState::Approach;
    game.step(InputFrame{}, kFixedStep);
    assert(!game.world().player.lock_on);
}

void testFacingHitboxRejectsRearTarget() {
    GameSimulation game;
    WorldState& world = game.mutableWorld();
    world.zone = Zone::Arena;
    world.player.position = {0.0f, 3.0f};
    world.player.facing = 3.14159265f;
    world.boss.position = {0.0f, 5.0f};
    world.boss.state = BossState::Recover;
    world.boss.state_timer = 5.0f;
    const float health_before = world.boss.health;
    InputFrame attack{};
    attack.light_attack = true;
    game.step(attack, kFixedStep);
    stepMany(game, InputFrame{}, 15);
    assert(game.world().boss.health == health_before);
}

void testDodgeInvulnerability() {
    GameSimulation game;
    WorldState& world = game.mutableWorld();
    world.zone = Zone::Arena;
    world.player.position = {0.0f, 0.0f};
    world.boss.position = {0.0f, 2.0f};
    world.boss.state = BossState::WindupSlash;
    world.boss.state_timer = 0.01f;
    InputFrame dodge{};
    dodge.move_x = 1.0f;
    dodge.dodge_pressed = true;
    game.step(dodge, kFixedStep);
    assert(game.world().player.health == 100.0f);
}

void testHealing() {
    GameSimulation game;
    game.mutableWorld().player.health = 40.0f;
    InputFrame heal{};
    heal.heal = true;
    game.step(heal, kFixedStep);
    assert(game.world().player.flasks == 2);
    stepMany(game, InputFrame{}, 24);
    assert(game.world().player.health > 80.0f);
}

void testBossDeathClearsLockAndProducesVictory() {
    GameSimulation game;
    WorldState& world = game.mutableWorld();
    world.zone = Zone::Arena;
    world.player.position = {0.0f, 2.8f};
    world.player.lock_on = true;
    world.boss.position = {0.0f, 5.0f};
    world.boss.health = 10.0f;
    world.boss.state = BossState::Recover;
    world.boss.state_timer = 5.0f;

    InputFrame attack{};
    attack.light_attack = true;
    game.step(attack, kFixedStep);
    stepMany(game, InputFrame{}, 15);
    assert(game.world().boss.state == BossState::Dead);
    assert(!game.world().player.lock_on);
    stepMany(game, InputFrame{}, 45);
    assert(game.world().player.state == PlayerState::Victory);
}

void testDeathAndRestart() {
    GameSimulation game;
    WorldState& world = game.mutableWorld();
    world.zone = Zone::Arena;
    world.player.health = 10.0f;
    world.player.position = {0.0f, 0.0f};
    world.boss.position = {0.0f, 2.0f};
    world.boss.state = BossState::WindupSlam;
    world.boss.state_timer = 0.01f;
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().player.state == PlayerState::Dead);
    InputFrame restart{};
    restart.interact = true;
    game.step(restart, kFixedStep);
    assert(game.world().zone == Zone::Interior);
    assert(game.world().player.health == 100.0f);
}

void testRepeatedArenaVictoryReset() {
    GameSimulation game;
    for (int cycle = 0; cycle < 5; ++cycle) {
        WorldState& world = game.mutableWorld();
        world.zone = Zone::Arena;
        world.player.position = {0.0f, 2.8f};
        world.player.facing = 0.0f;
        world.boss.position = {0.0f, 5.0f};
        world.boss.health = 10.0f;
        world.boss.state = BossState::Recover;
        world.boss.state_timer = 5.0f;
        InputFrame attack{};
        attack.light_attack = true;
        game.step(attack, kFixedStep);
        stepMany(game, InputFrame{}, 60);
        assert(game.world().player.state == PlayerState::Victory);
        InputFrame restart{};
        restart.interact = true;
        game.step(restart, kFixedStep);
        assert(game.world().zone == Zone::Interior);
        assert(ZoneManager::isLoaded(game.world(), Zone::Interior));
        assert(!ZoneManager::isLoaded(game.world(), Zone::Vista));
        assert(!ZoneManager::isLoaded(game.world(), Zone::Arena));
    }
}

} // namespace

int main() {
    testMathAndCollision();
    testGeneratedAssetRegistry();
    testGeneratedSceneData();
    testRigidPoseSampling();
    testMovementAndStamina();
    testQuickItemSelection();
    testZoneHandoffs();
    testDialogueAbortAndRetry();
    testSessionLifecycleAndPause();
    testCombatAndLockOn();
    testLockOnTargetLoss();
    testFacingHitboxRejectsRearTarget();
    testDodgeInvulnerability();
    testHealing();
    testBossDeathClearsLockAndProducesVictory();
    testDeathAndRestart();
    testRepeatedArenaVictoryReset();
    std::cout << "core_tests: all deterministic gameplay checks passed\n";
    return 0;
}
