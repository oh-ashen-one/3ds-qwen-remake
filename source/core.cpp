#include "demake/core.hpp"

#include <algorithm>
#include <cmath>

#include "demake/asset_registry.hpp"

namespace demake {
namespace {

constexpr float kPi = 3.14159265f;
constexpr Vec2 kDoor{0.0f, 4.3f};
constexpr Vec2 kNpc{0.0f, 15.5f};
constexpr Vec2 kFogGate{0.0f, 27.5f};

float clamp(float value, float low, float high) {
    return std::max(low, std::min(value, high));
}

float facingTo(Vec2 from, Vec2 to) {
    return std::atan2(to.x - from.x, to.z - from.z);
}

bool playerCanAct(PlayerState state) {
    return state == PlayerState::Idle || state == PlayerState::Move;
}

Vec2 forwardOf(float facing) {
    return {std::sin(facing), std::cos(facing)};
}

Vec2 offset(Vec2 origin, Vec2 direction, float amount) {
    return {origin.x + direction.x * amount, origin.z + direction.z * amount};
}

} // namespace

float length(Vec2 value) {
    return std::sqrt(value.x * value.x + value.z * value.z);
}

float distance(Vec2 a, Vec2 b) {
    return length({a.x - b.x, a.z - b.z});
}

Vec2 normalized(Vec2 value) {
    const float magnitude = length(value);
    if (magnitude < 0.0001f) {
        return {};
    }
    return {value.x / magnitude, value.z / magnitude};
}

bool circlesOverlap(Vec2 a, float radius_a, Vec2 b, float radius_b) {
    return distance(a, b) <= radius_a + radius_b;
}

float sampleRigidSwing(float normalized_time) {
    const float phase = clamp(normalized_time, 0.0f, 1.0f);
    return std::sin(phase * kPi) * 1.45f;
}

const char* quickItemName(int selected_item) {
    switch (selected_item) {
        case 0: return "Crimson Flask";
        case 1: return "Warden Effigy";
        case 2: return "Pale Moss";
        default: return "Unknown";
    }
}

void ZoneManager::reset(WorldState& world) const {
    world.zone = Zone::Interior;
    world.door_progress = 0.0f;
    world.transition_timer = 0.0f;
    world.door_activated = false;
    world.dialogue_active = false;
    world.dialogue_complete = false;
    world.arena_transition = false;
    world.loaded_zone_mask = 0;
    world.zone_resident_bytes = 0;
    world.zone_loads = 0;
    world.zone_unloads = 0;
    world.zone_transitions = 0;
    preload(world, Zone::Interior);
}

bool ZoneManager::isLoaded(const WorldState& world, Zone zone) {
    return (world.loaded_zone_mask & (1U << static_cast<unsigned>(zone))) != 0;
}

void ZoneManager::preload(WorldState& world, Zone zone) {
    if (isLoaded(world, zone)) {
        return;
    }
    world.loaded_zone_mask |= static_cast<std::uint8_t>(1U << static_cast<unsigned>(zone));
    world.zone_resident_bytes += AssetRegistry::zone(zone).runtime_budget_bytes;
    ++world.zone_loads;
}

void ZoneManager::unload(WorldState& world, Zone zone) {
    if (!isLoaded(world, zone)) {
        return;
    }
    world.loaded_zone_mask &= static_cast<std::uint8_t>(~(1U << static_cast<unsigned>(zone)));
    const std::uint32_t bytes = AssetRegistry::zone(zone).runtime_budget_bytes;
    world.zone_resident_bytes = world.zone_resident_bytes > bytes
                                    ? world.zone_resident_bytes - bytes
                                    : 0;
    ++world.zone_unloads;
}

void ZoneManager::enter(WorldState& world, Zone zone) {
    preload(world, zone);
    const Zone previous = world.zone;
    if (previous == zone) {
        return;
    }
    world.zone = zone;
    ++world.zone_transitions;
    unload(world, previous);
}

void ZoneManager::update(WorldState& world, const InputFrame& input, float dt) const {
    if (world.zone == Zone::Interior) {
        if (input.interact && distance(world.player.position, kDoor) < 2.2f) {
            world.door_activated = true;
            preload(world, Zone::Vista);
            world.player.state = PlayerState::Interact;
            world.player.state_timer = 0.45f;
        }
        if (world.door_activated) {
            world.door_progress = std::min(1.0f, world.door_progress + dt * 0.65f);
        }
        if (world.door_progress >= 0.95f && world.player.position.z > 5.1f) {
            enter(world, Zone::Vista);
            world.player.position.z = 6.2f;
        }
        return;
    }

    if (world.zone == Zone::Vista) {
        if (world.dialogue_active && input.dodge_pressed) {
            world.dialogue_active = false;
            world.player.state = PlayerState::Idle;
            world.player.state_timer = 0.0f;
        }
        if (input.interact && distance(world.player.position, kNpc) < 2.3f) {
            if (!world.dialogue_active && !world.dialogue_complete) {
                world.dialogue_active = true;
                world.player.state = PlayerState::Interact;
                world.player.state_timer = 0.35f;
            } else if (world.dialogue_active) {
                world.dialogue_active = false;
                world.dialogue_complete = true;
            }
        }
        if (input.interact && world.dialogue_complete &&
            distance(world.player.position, kFogGate) < 2.4f && !world.arena_transition) {
            preload(world, Zone::Arena);
            world.arena_transition = true;
            world.transition_timer = 0.85f;
            world.player.state = PlayerState::Interact;
            world.player.state_timer = 0.85f;
        }
        if (world.arena_transition) {
            world.transition_timer -= dt;
            if (world.transition_timer <= 0.0f) {
                enter(world, Zone::Arena);
                world.player.position = {0.0f, -5.5f};
                world.player.facing = 0.0f;
                world.player.state = PlayerState::Idle;
                world.player.state_timer = 0.0f;
                world.boss = Boss{};
                world.boss.state = BossState::Approach;
            }
        }
    }
}

const char* ZoneManager::name(Zone zone) {
    switch (zone) {
        case Zone::Interior: return "Sunken Vestibule";
        case Zone::Vista: return "The Sable Expanse";
        case Zone::Arena: return "Warden's Hollow";
    }
    return "Unknown";
}

GameSimulation::GameSimulation() {
    reset();
}

void GameSimulation::reset() {
    world_ = WorldState{};
    zones_.reset(world_);
}

void GameSimulation::step(const InputFrame& input, float dt) {
    dt = clamp(dt, 0.0f, 0.1f);
    world_.elapsed += dt;
    if (input.debug_toggle) {
        world_.debug_overlay = !world_.debug_overlay;
    }
    if (input.item_delta != 0) {
        constexpr int kQuickItemCount = 3;
        world_.player.selected_item =
            (world_.player.selected_item + input.item_delta + kQuickItemCount) % kQuickItemCount;
    }

    if (world_.player.state == PlayerState::Dead || world_.player.state == PlayerState::Victory) {
        if (input.interact) {
            reset();
        }
        return;
    }

    zones_.update(world_, input, dt);
    player_controller_.update(world_, input, dt);
    if (world_.zone == Zone::Arena) {
        boss_controller_.update(world_, dt);
    }
}

void PlayerController::update(WorldState& world_, const InputFrame& input, float dt) const {
    Player& player = world_.player;
    if (player.state_timer > 0.0f) {
        player.state_timer = std::max(0.0f, player.state_timer - dt);
    }

    if (player.state == PlayerState::Attack && !player.action_applied && player.state_timer <= 0.22f) {
        player.action_applied = true;
        const Vec2 hit_center = offset(player.position, forwardOf(player.facing), 1.25f);
        if (world_.zone == Zone::Arena &&
            circlesOverlap(hit_center, 1.25f, world_.boss.position, 0.9f)) {
            BossController::damage(world_, 16.0f);
        }
    } else if (player.state == PlayerState::HeavyAttack && !player.action_applied && player.state_timer <= 0.30f) {
        player.action_applied = true;
        const Vec2 hit_center = offset(player.position, forwardOf(player.facing), 1.45f);
        if (world_.zone == Zone::Arena &&
            circlesOverlap(hit_center, 1.55f, world_.boss.position, 0.9f)) {
            BossController::damage(world_, 30.0f);
        }
    } else if (player.state == PlayerState::Heal && !player.action_applied && player.state_timer <= 0.25f) {
        player.action_applied = true;
        player.health = std::min(100.0f, player.health + 48.0f);
    }

    finishTimedState(world_);

    if (!playerCanAct(player.state)) {
        return;
    }

    if (player.lock_on &&
        (world_.zone != Zone::Arena || world_.boss.state == BossState::Dead ||
         distance(player.position, world_.boss.position) > 14.0f)) {
        player.lock_on = false;
    }

    if (input.lock_toggle && world_.zone == Zone::Arena && world_.boss.state != BossState::Dead) {
        player.lock_on = !player.lock_on;
    }

    if (input.light_attack && player.stamina >= 12.0f && world_.zone == Zone::Arena) {
        player.state = PlayerState::Attack;
        player.state_timer = 0.46f;
        player.action_applied = false;
        player.stamina -= 12.0f;
        return;
    }
    if (input.heavy_attack && player.stamina >= 24.0f && world_.zone == Zone::Arena) {
        player.state = PlayerState::HeavyAttack;
        player.state_timer = 0.78f;
        player.action_applied = false;
        player.stamina -= 24.0f;
        return;
    }
    if (input.heal && player.flasks > 0 && player.health < 100.0f) {
        player.state = PlayerState::Heal;
        player.state_timer = 0.72f;
        player.action_applied = false;
        --player.flasks;
        return;
    }

    Vec2 movement{input.move_x, input.move_z};
    const float magnitude = length(movement);
    if (magnitude > 1.0f) {
        movement = normalized(movement);
    }

    if (input.dodge_pressed && magnitude > 0.15f && player.stamina >= 20.0f) {
        player.state = PlayerState::Dodge;
        player.state_timer = 0.48f;
        player.action_applied = false;
        player.stamina -= 20.0f;
        const Vec2 direction = normalized(movement);
        player.position.x += direction.x * 1.5f;
        player.position.z += direction.z * 1.5f;
    } else if (magnitude > 0.08f) {
        const bool sprinting = input.sprint_held && player.stamina > 1.0f;
        const float speed = sprinting ? 5.2f : 3.4f;
        player.position.x += movement.x * speed * dt;
        player.position.z += movement.z * speed * dt;
        player.state = PlayerState::Move;
        if (!player.lock_on) {
            player.facing = std::atan2(movement.x, movement.z);
        }
        if (sprinting) {
            player.stamina = std::max(0.0f, player.stamina - 18.0f * dt);
        }
    } else {
        player.state = PlayerState::Idle;
    }

    if (player.lock_on && world_.zone == Zone::Arena && world_.boss.state != BossState::Dead) {
        player.facing = facingTo(player.position, world_.boss.position);
    }

    if (!input.sprint_held && player.state != PlayerState::Attack &&
        player.state != PlayerState::HeavyAttack && player.state != PlayerState::Dodge) {
        player.stamina = std::min(100.0f, player.stamina + 22.0f * dt);
    }

    if (world_.zone == Zone::Interior) {
        player.position.x = clamp(player.position.x, -4.6f, 4.6f);
        const float forward_limit = world_.door_progress >= 0.8f ? 6.0f : 3.7f;
        player.position.z = clamp(player.position.z, -8.0f, forward_limit);
    } else if (world_.zone == Zone::Vista) {
        player.position.x = clamp(player.position.x, -11.0f, 11.0f);
        player.position.z = clamp(player.position.z, 5.8f, 28.2f);
    } else {
        player.position.x = clamp(player.position.x, -8.5f, 8.5f);
        player.position.z = clamp(player.position.z, -8.5f, 8.5f);
    }
}

void PlayerController::finishTimedState(WorldState& world_) {
    Player& player = world_.player;
    if (player.state_timer > 0.0f) {
        return;
    }
    switch (player.state) {
        case PlayerState::Attack:
        case PlayerState::HeavyAttack:
        case PlayerState::Dodge:
        case PlayerState::Hurt:
        case PlayerState::Heal:
        case PlayerState::Interact:
            player.state = PlayerState::Idle;
            player.action_applied = false;
            break;
        default:
            break;
    }
}

void BossController::update(WorldState& world_, float dt) const {
    Boss& boss = world_.boss;
    Player& player = world_.player;
    if (boss.state == BossState::Dead) {
        world_.victory_timer += dt;
        if (world_.victory_timer >= 1.4f) {
            player.state = PlayerState::Victory;
            player.lock_on = false;
        }
        return;
    }

    boss.facing = facingTo(boss.position, player.position);
    boss.state_timer = std::max(0.0f, boss.state_timer - dt);
    const float gap = distance(boss.position, player.position);

    switch (boss.state) {
        case BossState::Dormant:
            boss.state = BossState::Approach;
            break;
        case BossState::Approach: {
            if (gap > 2.8f) {
                const Vec2 direction = normalized({player.position.x - boss.position.x,
                                                   player.position.z - boss.position.z});
                boss.position.x += direction.x * 1.65f * dt;
                boss.position.z += direction.z * 1.65f * dt;
            } else {
                const bool slam = (boss.attack_cycle++ % 3U) == 2U;
                boss.state = slam ? BossState::WindupSlam : BossState::WindupSlash;
                boss.state_timer = slam ? 0.92f : 0.58f;
            }
            break;
        }
        case BossState::WindupSlash:
            if (boss.state_timer <= 0.0f) {
                boss.state = BossState::Slash;
                boss.state_timer = 0.18f;
                const Vec2 hit_center = offset(boss.position, forwardOf(boss.facing), 1.45f);
                if (circlesOverlap(hit_center, 1.35f, player.position, 0.55f) &&
                    player.state != PlayerState::Dodge) {
                    PlayerController::damage(world_, 24.0f);
                }
            }
            break;
        case BossState::WindupSlam:
            if (boss.state_timer <= 0.0f) {
                boss.state = BossState::Slam;
                boss.state_timer = 0.25f;
                if (circlesOverlap(boss.position, 3.25f, player.position, 0.55f) &&
                    player.state != PlayerState::Dodge) {
                    PlayerController::damage(world_, 34.0f);
                }
            }
            break;
        case BossState::Slash:
        case BossState::Slam:
            if (boss.state_timer <= 0.0f) {
                boss.state = BossState::Recover;
                boss.state_timer = 0.62f;
            }
            break;
        case BossState::Recover:
            if (boss.state_timer <= 0.0f) {
                boss.state = BossState::Approach;
            }
            break;
        case BossState::Dead:
            break;
    }
}

void PlayerController::damage(WorldState& world_, float amount) {
    Player& player = world_.player;
    if (player.state == PlayerState::Dodge || player.state == PlayerState::Dead) {
        return;
    }
    player.health = std::max(0.0f, player.health - amount);
    if (player.health <= 0.0f) {
        player.state = PlayerState::Dead;
        player.state_timer = 0.0f;
        player.lock_on = false;
    } else {
        player.state = PlayerState::Hurt;
        player.state_timer = 0.38f;
    }
}

void BossController::damage(WorldState& world_, float amount) {
    Boss& boss = world_.boss;
    if (boss.state == BossState::Dead) {
        return;
    }
    boss.health = std::max(0.0f, boss.health - amount);
    if (boss.health <= 0.0f) {
        boss.state = BossState::Dead;
        boss.state_timer = 0.0f;
        world_.player.lock_on = false;
    }
}

void GameSession::resetToTitle() {
    simulation_.reset();
    mode_ = SessionMode::Title;
    mode_before_suspend_ = SessionMode::Playing;
}

void GameSession::step(const InputFrame& input, float dt) {
    if (mode_ == SessionMode::Suspended) {
        return;
    }
    if (input.pause_toggle && mode_ != SessionMode::Title) {
        mode_ = mode_ == SessionMode::Paused ? SessionMode::Playing : SessionMode::Paused;
        return;
    }
    if (mode_ == SessionMode::Title) {
        if (input.interact) {
            simulation_.reset();
            mode_ = SessionMode::Playing;
        }
        return;
    }
    if (mode_ == SessionMode::Paused) {
        return;
    }
    simulation_.step(input, dt);
}

void GameSession::suspend() {
    if (mode_ == SessionMode::Suspended) {
        return;
    }
    mode_before_suspend_ = mode_;
    mode_ = SessionMode::Suspended;
}

void GameSession::resume() {
    if (mode_ == SessionMode::Suspended) {
        mode_ = mode_before_suspend_;
    }
}

} // namespace demake
