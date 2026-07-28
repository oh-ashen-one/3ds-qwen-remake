#include "demake/core.hpp"
#include "demake/asset_registry.hpp"
#include "demake/rigid_animation.hpp"
#include "demake/scene_assets.hpp"
#include "demake/zone_resources.hpp"

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
    assert(AssetRegistry::assetCount() == 20);
    const AssetRecord* exploration_music = AssetRegistry::find("music_ashen_deep_hall");
    assert(exploration_music != nullptr);
    assert(AssetRegistry::assetBelongsToZone(*exploration_music, Zone::Interior));
    assert(AssetRegistry::assetBelongsToZone(*exploration_music, Zone::Vista));
    assert(!AssetRegistry::assetBelongsToZone(*exploration_music, Zone::Arena));
    assert(!AssetRegistry::assetBelongsToZone(*exploration_music, Zone::Field));
    const AssetRecord* boss_music = AssetRegistry::find("music_ashen_gate");
    assert(boss_music != nullptr);
    assert(!AssetRegistry::assetBelongsToZone(*boss_music, Zone::Interior));
    assert(!AssetRegistry::assetBelongsToZone(*boss_music, Zone::Vista));
    assert(AssetRegistry::assetBelongsToZone(*boss_music, Zone::Arena));
    assert(AssetRegistry::assetBelongsToZone(*boss_music, Zone::BoarValley));
    assert(AssetRegistry::assetBelongsToZone(*boss_music, Zone::CloudPlateau));
    const AssetRecord* field_music = AssetRegistry::find("music_valley_after_dawn");
    assert(field_music != nullptr);
    assert(!AssetRegistry::assetBelongsToZone(*field_music, Zone::Interior));
    assert(!AssetRegistry::assetBelongsToZone(*field_music, Zone::Arena));
    assert(AssetRegistry::assetBelongsToZone(*field_music, Zone::Field));
    const AssetRecord* dialogue = AssetRegistry::find("veiled_keeper_dialogue");
    assert(dialogue != nullptr);
    assert(!AssetRegistry::assetBelongsToZone(*dialogue, Zone::Interior));
    assert(AssetRegistry::assetBelongsToZone(*dialogue, Zone::Vista));
    const AssetRecord* texture = AssetRegistry::find("environment_texture_atlas");
    assert(texture != nullptr);
    assert(std::strcmp(texture->kind, "texture_atlas") == 0);
    assert(AssetRegistry::assetBelongsToZone(*texture, Zone::Arena));
    assert(AssetRegistry::assetBelongsToZone(*texture, Zone::Field));
    const AssetRecord* interior_blob = AssetRegistry::find("interior_scene_blob");
    assert(interior_blob != nullptr);
    assert(AssetRegistry::assetBelongsToZone(*interior_blob, Zone::Interior));
    assert(!AssetRegistry::assetBelongsToZone(*interior_blob, Zone::Vista));
    assert(AssetRegistry::zone(Zone::Arena).draw_call_budget == 84);
    const AssetRecord* field_blob = AssetRegistry::find("field_scene_blob");
    assert(field_blob != nullptr);
    assert(AssetRegistry::assetBelongsToZone(*field_blob, Zone::Field));
    assert(AssetRegistry::zone(Zone::Field).draw_call_budget == 204);
    const AssetRecord* boar_blob = AssetRegistry::find("boar_valley_scene_blob");
    assert(boar_blob != nullptr);
    assert(AssetRegistry::assetBelongsToZone(*boar_blob, Zone::BoarValley));
    const AssetRecord* cloud_blob = AssetRegistry::find("cloud_plateau_scene_blob");
    assert(cloud_blob != nullptr);
    assert(AssetRegistry::assetBelongsToZone(*cloud_blob, Zone::CloudPlateau));
    assert(AssetRegistry::zone(Zone::BoarValley).draw_call_budget == 188);
    assert(AssetRegistry::zone(Zone::CloudPlateau).draw_call_budget == 214);
    assert(AssetRegistry::find("missing") == nullptr);
}

void testGeneratedSceneData() {
    assert(SceneAssets::boxCount(Zone::Interior) == 16);
    assert(SceneAssets::boxCount(Zone::Vista) == 24);
    assert(SceneAssets::boxCount(Zone::Arena) == 13);
    assert(SceneAssets::boxCount(Zone::Field) == 384);
    assert(SceneAssets::boxCount(Zone::BoarValley) == 142);
    assert(SceneAssets::boxCount(Zone::CloudPlateau) == 144);
    std::size_t count = 0;
    const SceneBox* arena = SceneAssets::boxes(Zone::Arena, count);
    assert(arena != nullptr && count == 13);
    assert(arena[0].sx > 0.0f && arena[0].sy > 0.0f && arena[0].sz > 0.0f);
}

void testZoneResourceStreaming() {
    ZoneResources resources;
    assert(resources.loadedMask() == 0);
    assert(resources.residentBytes() == 0);

    assert(resources.sync(1U << static_cast<unsigned>(Zone::Interior)));
    assert(resources.loadedMask() == 0x01U);
    assert(resources.loadCount() == 1);
    assert(resources.unloadCount() == 0);
    std::size_t count = 0;
    const SceneBox* interior = resources.boxes(Zone::Interior, count);
    assert(interior != nullptr && count == SceneAssets::boxCount(Zone::Interior));
    assert(resources.residentBytes() == count * sizeof(SceneBox));

    assert(resources.sync(0x03U));
    assert(resources.loadedMask() == 0x03U);
    assert(resources.loadCount() == 2);
    const std::uint32_t overlap_bytes = resources.residentBytes();
    assert(overlap_bytes ==
           (SceneAssets::boxCount(Zone::Interior) + SceneAssets::boxCount(Zone::Vista)) *
               sizeof(SceneBox));

    assert(resources.sync(1U << static_cast<unsigned>(Zone::Vista)));
    assert(resources.loadedMask() == 0x02U);
    assert(resources.loadCount() == 2);
    assert(resources.unloadCount() == 1);
    assert(resources.boxes(Zone::Interior, count) == nullptr && count == 0);
    assert(resources.residentBytes() == SceneAssets::boxCount(Zone::Vista) * sizeof(SceneBox));

    const std::uint8_t stable_mask = resources.loadedMask();
    const std::uint32_t stable_bytes = resources.residentBytes();
    assert(!resources.sync(0x80U));
    assert(resources.loadedMask() == stable_mask);
    assert(resources.residentBytes() == stable_bytes);

    assert(resources.sync(1U << static_cast<unsigned>(Zone::Arena)));
    assert(resources.loadedMask() == 0x04U);
    assert(resources.boxes(Zone::Arena, count) != nullptr);
    assert(count == SceneAssets::boxCount(Zone::Arena));
    assert(resources.loadCount() == 3);
    assert(resources.unloadCount() == 2);

    assert(resources.sync(1U << static_cast<unsigned>(Zone::Field)));
    assert(resources.loadedMask() == 0x08U);
    assert(resources.boxes(Zone::Field, count) != nullptr);
    assert(count == SceneAssets::boxCount(Zone::Field));
    assert(resources.loadCount() == 4);
    assert(resources.unloadCount() == 3);

    assert(resources.sync(1U << static_cast<unsigned>(Zone::BoarValley)));
    assert(resources.loadedMask() == 0x10U);
    assert(resources.boxes(Zone::BoarValley, count) != nullptr);
    assert(count == SceneAssets::boxCount(Zone::BoarValley));

    assert(resources.sync(1U << static_cast<unsigned>(Zone::CloudPlateau)));
    assert(resources.loadedMask() == 0x20U);
    assert(resources.boxes(Zone::CloudPlateau, count) != nullptr);
    assert(count == SceneAssets::boxCount(Zone::CloudPlateau));

    resources.shutdown();
    assert(resources.loadedMask() == 0);
    assert(resources.residentBytes() == 0);
    assert(resources.unloadCount() == 6);
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

void testControllerDamageBoundaries() {
    WorldState world{};
    PlayerController::damage(world, 25.0f);
    assert(world.player.health == 75.0f);
    assert(world.player.state == PlayerState::Hurt);

    world.player.state = PlayerState::Dodge;
    PlayerController::damage(world, 25.0f);
    assert(world.player.health == 75.0f);

    BossController::damage(world, 60.0f);
    assert(world.boss.health == 80.0f);
    BossController::damage(world, 250.0f);
    assert(world.boss.health == 0.0f);
    assert(world.boss.state == BossState::Dead);
}

void testPlayerTimedStateTransitions() {
    PlayerController controller;
    WorldState world{};
    world.zone = Zone::Arena;
    world.player.position = {0.0f, 0.0f};
    world.boss.position = {0.0f, 8.0f};

    InputFrame light{};
    light.light_attack = true;
    controller.update(world, light, kFixedStep);
    assert(world.player.state == PlayerState::Attack);
    for (int frame = 0; frame < 16; ++frame) {
        controller.update(world, InputFrame{}, kFixedStep);
    }
    assert(world.player.state == PlayerState::Idle);

    InputFrame heavy{};
    heavy.heavy_attack = true;
    controller.update(world, heavy, kFixedStep);
    assert(world.player.state == PlayerState::HeavyAttack);
    for (int frame = 0; frame < 25; ++frame) {
        controller.update(world, InputFrame{}, kFixedStep);
    }
    assert(world.player.state == PlayerState::Idle);

    InputFrame dodge{};
    dodge.move_x = 1.0f;
    dodge.dodge_pressed = true;
    controller.update(world, dodge, kFixedStep);
    assert(world.player.state == PlayerState::Dodge);
    for (int frame = 0; frame < 16; ++frame) {
        controller.update(world, InputFrame{}, kFixedStep);
    }
    assert(world.player.state == PlayerState::Idle);

    PlayerController::damage(world, 10.0f);
    assert(world.player.state == PlayerState::Hurt);
    for (int frame = 0; frame < 13; ++frame) {
        controller.update(world, InputFrame{}, kFixedStep);
    }
    assert(world.player.state == PlayerState::Idle);
}

void testBossStateTransitions() {
    BossController controller;
    WorldState world{};
    world.zone = Zone::Arena;
    world.player.position = {0.0f, 0.0f};
    world.boss.position = {0.0f, 2.0f};
    world.boss.state = BossState::Approach;

    controller.update(world, kFixedStep);
    assert(world.boss.state == BossState::WindupSlash);
    int guard = 0;
    while (world.boss.state == BossState::WindupSlash && guard++ < 30) {
        controller.update(world, kFixedStep);
    }
    assert(world.boss.state == BossState::Slash);
    guard = 0;
    while (world.boss.state == BossState::Slash && guard++ < 15) {
        controller.update(world, kFixedStep);
    }
    assert(world.boss.state == BossState::Recover);
    guard = 0;
    while (world.boss.state == BossState::Recover && guard++ < 30) {
        controller.update(world, kFixedStep);
    }
    assert(world.boss.state == BossState::Approach);

    world.boss.attack_cycle = 2;
    controller.update(world, kFixedStep);
    assert(world.boss.state == BossState::WindupSlam);
    guard = 0;
    while (world.boss.state == BossState::WindupSlam && guard++ < 40) {
        controller.update(world, kFixedStep);
    }
    assert(world.boss.state == BossState::Slam);
    guard = 0;
    while (world.boss.state == BossState::Slam && guard++ < 15) {
        controller.update(world, kFixedStep);
    }
    assert(world.boss.state == BossState::Recover);
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

    WorldState& arena_world = game.mutableWorld();
    arena_world.boss.state = BossState::Dead;
    arena_world.player.state = PlayerState::Victory;
    game.step(interact, kFixedStep);
    assert(arena_world.field_transition);
    assert(ZoneManager::isLoaded(arena_world, Zone::Arena));
    assert(ZoneManager::isLoaded(arena_world, Zone::Field));
    stepMany(game, InputFrame{}, 33);
    assert(game.world().zone == Zone::Field);
    assert(!ZoneManager::isLoaded(game.world(), Zone::Arena));
    assert(ZoneManager::isLoaded(game.world(), Zone::Field));
    assert(game.world().zone_unloads == 3);
    assert(game.world().zone_transitions == 3);
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

void testSunlitReachHorseMountAndGallop() {
    GameSimulation game;
    WorldState& world = game.mutableWorld();
    world.zone = Zone::Arena;
    world.loaded_zone_mask = 1U << static_cast<unsigned>(Zone::Arena);
    world.boss.state = BossState::Dead;
    world.player.state = PlayerState::Victory;

    InputFrame interact{};
    interact.interact = true;
    game.step(interact, kFixedStep);
    stepMany(game, InputFrame{}, 33);
    assert(game.world().zone == Zone::Field);
    assert(!game.world().player.mounted);

    assert(game.world().horses.size() == kFieldHorseCount);
    game.mutableWorld().player.position = game.world().horses[1].position;
    game.step(interact, kFixedStep);
    assert(game.world().player.mounted);
    assert(game.world().active_horse == 1);
    stepMany(game, InputFrame{}, 10);

    const float start_z = game.world().player.position.z;
    InputFrame gallop{};
    gallop.move_z = 1.0f;
    gallop.sprint_held = true;
    stepMany(game, gallop, 30);
    assert(game.world().player.position.z > start_z + 9.5f);
    assert(distance(game.world().player.position, game.world().horses[1].position) < 0.001f);
    assert(game.world().player.stamina < 100.0f);

    game.mutableWorld().player.health = 40.0f;
    InputFrame mounted_heal{};
    mounted_heal.heal = true;
    game.step(mounted_heal, kFixedStep);
    assert(game.world().player.flasks == 2);
    stepMany(game, InputFrame{}, 24);
    assert(game.world().player.health > 80.0f);
    assert(game.world().player.mounted);

    game.step(interact, kFixedStep);
    assert(!game.world().player.mounted);
    assert(distance(game.world().player.position, game.world().horses[1].position) > 1.0f);

    stepMany(game, InputFrame{}, 10);
    game.mutableWorld().player.position = {12.0f, 12.0f};
    InputFrame call{};
    call.lock_toggle = true;
    game.step(call, kFixedStep);
    assert(distance(game.world().player.position, game.world().horses[1].position) < 2.0f);
}

void testOutdoorBranchesAndPersistentBosses() {
    GameSimulation game;
    WorldState& world = game.mutableWorld();
    world.zone = Zone::Field;
    world.loaded_zone_mask = 1U << static_cast<unsigned>(Zone::Field);
    world.player.position = {69.0f, 18.0f};
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().zone == Zone::BoarValley);
    assert(game.world().boss.health == bossMaximumHealth(Zone::BoarValley));
    assert(!game.world().player.mounted);
    BossController::damage(game.mutableWorld(), 1000.0f);
    assert(game.world().boar_defeated);
    game.mutableWorld().player.position.z = -42.0f;
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().zone == Zone::Field);
    game.mutableWorld().player.position = {69.0f, 18.0f};
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().zone == Zone::BoarValley);
    assert(game.world().boss.state == BossState::Dead);

    game.mutableWorld().player.position.z = -42.0f;
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().zone == Zone::Field);
    game.mutableWorld().player.position = {-69.0f, 18.0f};
    game.mutableWorld().player.mounted = true;
    game.mutableWorld().active_horse = 0;
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().zone == Zone::CloudPlateau);
    assert(game.world().player.mounted);
    assert(zoneGroundHeight(Zone::CloudPlateau, {0.0f, 76.0f}) == 26.0f);

    game.mutableWorld().player.position = {0.0f, 84.0f};
    game.step(InputFrame{}, kFixedStep);
    assert(!game.world().player.mounted);
    game.mutableWorld().player.position = {0.0f, 101.0f};
    game.mutableWorld().boss.position = {0.0f, 105.0f};
    game.mutableWorld().boss.state = BossState::Approach;
    game.mutableWorld().boss.attack_cycle = 3;
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().boss.state == BossState::WindupMagic);
    game.mutableWorld().boss.state_timer = 0.01f;
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().boss.state == BossState::Magic);

    BossController::damage(game.mutableWorld(), 1000.0f);
    assert(game.world().ogre_defeated);
    game.mutableWorld().player.position.z = -46.0f;
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().zone == Zone::Field);
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

} // namespace

int main() {
    testMathAndCollision();
    testGeneratedAssetRegistry();
    testGeneratedSceneData();
    testZoneResourceStreaming();
    testRigidPoseSampling();
    testMovementAndStamina();
    testControllerDamageBoundaries();
    testPlayerTimedStateTransitions();
    testBossStateTransitions();
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
    testSunlitReachHorseMountAndGallop();
    testOutdoorBranchesAndPersistentBosses();
    testDeathAndRestart();
    std::cout << "core_tests: all deterministic gameplay checks passed\n";
    return 0;
}
