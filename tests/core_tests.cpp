#include "demake/core.hpp"

#include <cassert>
#include <cmath>
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

void testMovementAndStamina() {
    GameSimulation game;
    InputFrame move{};
    move.move_z = 1.0f;
    move.sprint_held = true;
    stepMany(game, move, 20);
    assert(game.world().player.position.z > -4.0f);
    assert(game.world().player.stamina < 100.0f);
}

void testZoneHandoffs() {
    GameSimulation game;
    game.mutableWorld().player.position = {0.0f, 3.5f};
    InputFrame interact{};
    interact.interact = true;
    game.step(interact, kFixedStep);
    stepMany(game, InputFrame{}, 55);
    game.mutableWorld().player.position.z = 5.4f;
    game.step(InputFrame{}, kFixedStep);
    assert(game.world().zone == Zone::Vista);

    game.mutableWorld().player.position = {0.0f, 15.5f};
    game.step(interact, kFixedStep);
    game.step(InputFrame{}, kFixedStep);
    game.step(interact, kFixedStep);
    assert(game.world().dialogue_complete);
    game.mutableWorld().player.position = {0.0f, 27.5f};
    game.step(interact, kFixedStep);
    stepMany(game, InputFrame{}, 30);
    assert(game.world().zone == Zone::Arena);
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

} // namespace

int main() {
    testMathAndCollision();
    testMovementAndStamina();
    testZoneHandoffs();
    testCombatAndLockOn();
    testDodgeInvulnerability();
    testHealing();
    testBossDeathClearsLockAndProducesVictory();
    testDeathAndRestart();
    std::cout << "core_tests: all deterministic gameplay checks passed\n";
    return 0;
}
