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
constexpr Vec2 kFieldSpawn{0.0f, -38.0f};
constexpr Vec2 kBoarSpawn{0.0f, -37.0f};
constexpr Vec2 kOgreClimbSpawn{0.0f, -42.0f};
constexpr float kBranchTransitionDuration = 0.72f;
constexpr float kArrivalFadeDuration = 0.42f;
constexpr std::array<FieldHorse, kFieldHorseCount> kFieldHorseSpawns{{
    {{2.0f, -28.0f}, 0.0f},
    {{-38.0f, 26.0f}, 1.15f},
    {{36.0f, 68.0f}, -0.80f},
}};

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

unsigned nearestHorseIndex(const WorldState& world) {
    unsigned nearest = 0;
    float nearest_distance = distance(world.player.position, world.horses[0].position);
    for (unsigned index = 1; index < kFieldHorseCount; ++index) {
        const float candidate = distance(world.player.position, world.horses[index].position);
        if (candidate < nearest_distance) {
            nearest = index;
            nearest_distance = candidate;
        }
    }
    return nearest;
}

void prepareBoss(WorldState& world, Zone zone) {
    world.boss = Boss{};
    world.boss.state = BossState::Dormant;
    world.boss.health = bossMaximumHealth(zone);
    world.boss.position = zone == Zone::BoarValley ? Vec2{0.0f, 29.0f}
                                                   : Vec2{0.0f, 105.0f};
    const bool already_defeated =
        (zone == Zone::BoarValley && world.boar_defeated) ||
        (zone == Zone::CloudPlateau && world.ogre_defeated);
    if (already_defeated) {
        world.boss.health = 0.0f;
        world.boss.state = BossState::Dead;
    }
    world.victory_timer = already_defeated ? 99.0f : 0.0f;
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

bool isBossZone(Zone zone) {
    return zone == Zone::Arena || zone == Zone::BoarValley ||
           zone == Zone::CloudPlateau;
}

float bossMaximumHealth(Zone zone) {
    switch (zone) {
        case Zone::Arena: return 140.0f;
        case Zone::BoarValley: return 105.0f;
        case Zone::CloudPlateau: return 185.0f;
        default: return 1.0f;
    }
}

const char* bossDisplayName(Zone zone) {
    switch (zone) {
        case Zone::Arena: return "THE ASHEN WARDEN";
        case Zone::BoarValley: return "GORE-TUSK, VALLEY KING";
        case Zone::CloudPlateau: return "ARASHI, MOUNTAIN OGRE";
        default: return "";
    }
}

float zoneGroundHeight(Zone zone, Vec2 position) {
    if (zone != Zone::CloudPlateau) {
        return 0.0f;
    }
    if (position.z <= -28.0f) {
        return 0.0f;
    }
    if (position.z >= 76.0f) {
        return 26.0f;
    }
    return (position.z + 28.0f) * 0.25f;
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
    world.arrival_fade_timer = 0.0f;
    world.door_activated = false;
    world.dialogue_active = false;
    world.dialogue_complete = false;
    world.arena_transition = false;
    world.field_transition = false;
    world.branch_transition = false;
    world.pending_zone = Zone::Interior;
    world.boar_defeated = false;
    world.ogre_defeated = false;
    world.horses = kFieldHorseSpawns;
    world.active_horse = 0;
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

void ZoneManager::beginBranchTransition(WorldState& world, Zone target) {
    preload(world, target);
    world.pending_zone = target;
    world.branch_transition = true;
    world.transition_timer = kBranchTransitionDuration;
    world.player.state = PlayerState::Interact;
    world.player.state_timer = kBranchTransitionDuration;
    world.player.lock_on = false;
}

void ZoneManager::beginPostBossField(WorldState& world) const {
    if (world.zone != Zone::Arena || world.boss.state != BossState::Dead ||
        world.field_transition) {
        return;
    }
    preload(world, Zone::Field);
    world.field_transition = true;
    world.transition_timer = 1.05f;
    world.player.state = PlayerState::Interact;
    world.player.state_timer = 1.05f;
}

void ZoneManager::update(WorldState& world, const InputFrame& input, float dt) const {
    world.arrival_fade_timer = std::max(0.0f, world.arrival_fade_timer - dt);

    if (world.branch_transition) {
        world.transition_timer -= dt;
        if (world.transition_timer > 0.0f) {
            return;
        }

        const Zone previous = world.zone;
        const Zone target = world.pending_zone;
        const bool transfer_mount = world.player.mounted;
        enter(world, target);
        world.player.state = PlayerState::Idle;
        world.player.state_timer = 0.0f;
        world.player.lock_on = false;
        world.branch_transition = false;
        world.arrival_fade_timer = kArrivalFadeDuration;

        if (target == Zone::BoarValley) {
            world.player.position = kBoarSpawn;
            world.player.facing = 0.0f;
            world.player.mounted = false;
            prepareBoss(world, Zone::BoarValley);
        } else if (target == Zone::CloudPlateau) {
            world.player.position = kOgreClimbSpawn;
            world.player.facing = 0.0f;
            world.player.mounted = transfer_mount;
            FieldHorse& horse = world.horses[world.active_horse];
            horse.position = transfer_mount
                                 ? world.player.position
                                 : Vec2{world.player.position.x + 1.8f,
                                        world.player.position.z};
            horse.facing = world.player.facing;
            prepareBoss(world, Zone::CloudPlateau);
        } else if (target == Zone::Field) {
            if (previous == Zone::BoarValley) {
                world.player.position = {66.0f, 18.0f};
                world.player.facing = -1.5707963f;
                world.player.mounted = false;
            } else {
                world.player.position = {-66.0f, 18.0f};
                world.player.facing = 1.5707963f;
                world.player.mounted = transfer_mount;
                FieldHorse& horse = world.horses[world.active_horse];
                horse.position = transfer_mount
                                     ? world.player.position
                                     : Vec2{world.player.position.x + 1.8f,
                                            world.player.position.z};
                horse.facing = world.player.facing;
            }
        }
        return;
    }

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
                world.arena_transition = false;
                world.arrival_fade_timer = kArrivalFadeDuration;
            }
        }
        return;
    }

    if (world.zone == Zone::Arena) {
        if (world.field_transition) {
            world.transition_timer -= dt;
            if (world.transition_timer <= 0.0f) {
                enter(world, Zone::Field);
                world.player.position = kFieldSpawn;
                world.player.facing = 0.0f;
                world.player.state = PlayerState::Idle;
                world.player.state_timer = 0.0f;
                world.player.mounted = false;
                world.player.health = 100.0f;
                world.player.stamina = 100.0f;
                world.player.flasks = 3;
                world.horses = kFieldHorseSpawns;
                world.active_horse = 0;
                world.field_transition = false;
                world.arrival_fade_timer = kArrivalFadeDuration;
            }
        }
        return;
    }

    if (world.zone == Zone::Field && playerCanAct(world.player.state)) {
        if (world.player.position.x > 68.0f &&
            world.player.position.z > -8.0f && world.player.position.z < 54.0f) {
            beginBranchTransition(world, Zone::BoarValley);
            return;
        }
        if (world.player.position.x < -68.0f &&
            world.player.position.z > -8.0f && world.player.position.z < 54.0f) {
            beginBranchTransition(world, Zone::CloudPlateau);
            return;
        }
        if (input.lock_toggle && !world.player.mounted) {
            const Vec2 forward = forwardOf(world.player.facing);
            FieldHorse& horse = world.horses[world.active_horse];
            horse.position = offset(world.player.position, forward, 1.8f);
            horse.facing = world.player.facing;
        }
        if (input.interact) {
            if (world.player.mounted) {
                const Vec2 side{std::cos(world.player.facing), -std::sin(world.player.facing)};
                FieldHorse& horse = world.horses[world.active_horse];
                world.player.mounted = false;
                horse.position = world.player.position;
                horse.facing = world.player.facing;
                world.player.position.x += side.x * 1.35f;
                world.player.position.z += side.z * 1.35f;
            } else {
                const unsigned nearest = nearestHorseIndex(world);
                if (distance(world.player.position, world.horses[nearest].position) < 2.6f) {
                    world.active_horse = nearest;
                    world.player.mounted = true;
                    world.player.position = world.horses[nearest].position;
                    world.player.facing = world.horses[nearest].facing;
                }
            }
            world.player.state = PlayerState::Interact;
            world.player.state_timer = 0.28f;
        }
        return;
    }

    if (world.zone == Zone::BoarValley) {
        if (world.player.position.z < -40.0f) {
            beginBranchTransition(world, Zone::Field);
        }
        return;
    }

    if (world.zone == Zone::CloudPlateau) {
        if (world.player.position.z < -44.0f) {
            beginBranchTransition(world, Zone::Field);
            return;
        }
        if (world.player.position.z > 82.0f && world.player.mounted) {
            FieldHorse& horse = world.horses[world.active_horse];
            horse.position = {world.player.position.x - 2.0f, world.player.position.z - 1.5f};
            horse.facing = world.player.facing;
            world.player.mounted = false;
        }
        if (playerCanAct(world.player.state)) {
            if (input.lock_toggle && !world.player.mounted) {
                FieldHorse& horse = world.horses[world.active_horse];
                const Vec2 forward = forwardOf(world.player.facing);
                horse.position = offset(world.player.position, forward, 1.8f);
                horse.facing = world.player.facing;
            }
            if (input.interact) {
                FieldHorse& horse = world.horses[world.active_horse];
                if (world.player.mounted) {
                    world.player.mounted = false;
                    horse.position = world.player.position;
                    horse.facing = world.player.facing;
                    world.player.position.x += 1.35f;
                } else if (distance(world.player.position, horse.position) < 2.6f) {
                    world.player.mounted = true;
                    world.player.position = horse.position;
                    world.player.facing = horse.facing;
                }
                world.player.state = PlayerState::Interact;
                world.player.state_timer = 0.28f;
            }
        }
    }
}

const char* ZoneManager::name(Zone zone) {
    switch (zone) {
        case Zone::Interior: return "Sunken Vestibule";
        case Zone::Vista: return "The Sable Expanse";
        case Zone::Arena: return "Warden's Hollow";
        case Zone::Field: return "The Sunlit Reach";
        case Zone::BoarValley: return "Twinfang Ravine";
        case Zone::CloudPlateau: return "Cloudbreak Ascent";
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

    if (world_.player.state == PlayerState::Dead) {
        if (input.interact) {
            restartFromCheckpoint();
        }
        return;
    }
    if (world_.player.state == PlayerState::Victory) {
        if (input.interact) {
            zones_.beginPostBossField(world_);
        }
        return;
    }

    zones_.update(world_, input, dt);
    player_controller_.update(world_, input, dt);
    if (isBossZone(world_.zone) && !world_.field_transition &&
        !world_.branch_transition) {
        boss_controller_.update(world_, dt);
    }
}

void GameSimulation::restartFromCheckpoint() {
    const Zone checkpoint = world_.zone;
    if (!isBossZone(checkpoint) && checkpoint != Zone::Field) {
        reset();
        return;
    }

    const bool debug_overlay = world_.debug_overlay;
    const bool boar_defeated = world_.boar_defeated;
    const bool ogre_defeated = world_.ogre_defeated;
    const unsigned active_horse = world_.active_horse;
    const auto horses = world_.horses;
    world_.player = Player{};
    world_.debug_overlay = debug_overlay;
    world_.boar_defeated = boar_defeated;
    world_.ogre_defeated = ogre_defeated;
    world_.active_horse = active_horse;
    world_.horses = horses;
    world_.transition_timer = 0.0f;
    world_.arrival_fade_timer = kArrivalFadeDuration;
    world_.arena_transition = false;
    world_.field_transition = false;
    world_.branch_transition = false;
    world_.pending_zone = checkpoint;

    if (checkpoint == Zone::Arena) {
        world_.player.position = {0.0f, -5.5f};
        world_.boss = Boss{};
        world_.boss.state = BossState::Approach;
    } else if (checkpoint == Zone::BoarValley) {
        world_.player.position = kBoarSpawn;
        prepareBoss(world_, Zone::BoarValley);
    } else if (checkpoint == Zone::CloudPlateau) {
        world_.player.position = kOgreClimbSpawn;
        world_.player.mounted = true;
        world_.horses[world_.active_horse].position = kOgreClimbSpawn;
        world_.horses[world_.active_horse].facing = 0.0f;
        prepareBoss(world_, Zone::CloudPlateau);
    } else {
        world_.player.position = kFieldSpawn;
        world_.horses = kFieldHorseSpawns;
        world_.active_horse = 0;
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
        const float boss_radius = world_.zone == Zone::BoarValley ? 1.45f
                                  : (world_.zone == Zone::CloudPlateau ? 1.35f : 0.9f);
        if (isBossZone(world_.zone) &&
            circlesOverlap(hit_center, 1.25f, world_.boss.position, boss_radius)) {
            BossController::damage(world_, 20.0f);
        }
    } else if (player.state == PlayerState::HeavyAttack && !player.action_applied && player.state_timer <= 0.30f) {
        player.action_applied = true;
        const Vec2 hit_center = offset(player.position, forwardOf(player.facing), 1.45f);
        const float boss_radius = world_.zone == Zone::BoarValley ? 1.45f
                                  : (world_.zone == Zone::CloudPlateau ? 1.35f : 0.9f);
        if (isBossZone(world_.zone) &&
            circlesOverlap(hit_center, 1.55f, world_.boss.position, boss_radius)) {
            BossController::damage(world_, 38.0f);
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
        (!isBossZone(world_.zone) || world_.boss.state == BossState::Dead ||
         distance(player.position, world_.boss.position) > 18.0f)) {
        player.lock_on = false;
    }

    if (input.lock_toggle && isBossZone(world_.zone) &&
        world_.boss.state != BossState::Dead &&
        distance(player.position, world_.boss.position) <= 18.0f) {
        player.lock_on = !player.lock_on;
    }

    if (!player.mounted && input.light_attack && player.stamina >= 12.0f &&
        isBossZone(world_.zone)) {
        player.state = PlayerState::Attack;
        player.state_timer = 0.46f;
        player.action_applied = false;
        player.stamina -= 12.0f;
        return;
    }
    if (!player.mounted && input.heavy_attack && player.stamina >= 24.0f &&
        isBossZone(world_.zone)) {
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

    if (!player.mounted && input.dodge_pressed && magnitude > 0.15f &&
        player.stamina >= 20.0f) {
        player.state = PlayerState::Dodge;
        player.state_timer = 0.48f;
        player.action_applied = false;
        player.stamina -= 20.0f;
        const Vec2 direction = normalized(movement);
        player.position.x += direction.x * 1.7f;
        player.position.z += direction.z * 1.7f;
    } else if (magnitude > 0.08f) {
        const bool sprinting = input.sprint_held && player.stamina > 1.0f;
        const float speed = player.mounted ? (sprinting ? 10.2f : 7.0f)
                                           : (sprinting ? 5.2f : 3.4f);
        player.position.x += movement.x * speed * dt;
        player.position.z += movement.z * speed * dt;
        player.state = PlayerState::Move;
        if (!player.lock_on) {
            player.facing = std::atan2(movement.x, movement.z);
        }
        if (sprinting) {
            const float stamina_cost = player.mounted ? 10.0f : 18.0f;
            player.stamina = std::max(0.0f, player.stamina - stamina_cost * dt);
        }
    } else {
        player.state = PlayerState::Idle;
    }

    if (player.lock_on && isBossZone(world_.zone) &&
        world_.boss.state != BossState::Dead) {
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
    } else if (world_.zone == Zone::Arena) {
        player.position.x = clamp(player.position.x, -8.5f, 8.5f);
        player.position.z = clamp(player.position.z, -8.5f, 8.5f);
    } else if (world_.zone == Zone::BoarValley) {
        player.position.x = clamp(player.position.x, -25.0f, 25.0f);
        player.position.z = clamp(player.position.z, -43.0f, 57.0f);
    } else if (world_.zone == Zone::CloudPlateau) {
        const float path_half_width = player.position.z < 76.0f ? 12.0f : 34.0f;
        player.position.x = clamp(player.position.x, -path_half_width, path_half_width);
        player.position.z = clamp(player.position.z, -47.0f, 126.0f);
        if (player.mounted) {
            FieldHorse& horse = world_.horses[world_.active_horse];
            horse.position = player.position;
            horse.facing = player.facing;
        }
    } else {
        player.position.x = clamp(player.position.x, -72.0f, 72.0f);
        player.position.z = clamp(player.position.z, -42.0f, 132.0f);
        if (player.mounted) {
            FieldHorse& horse = world_.horses[world_.active_horse];
            horse.position = player.position;
            horse.facing = player.facing;
        }
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
        if (world_.zone == Zone::Arena && world_.victory_timer >= 1.4f) {
            player.state = PlayerState::Victory;
            player.lock_on = false;
        }
        return;
    }

    boss.facing = facingTo(boss.position, player.position);
    boss.state_timer = std::max(0.0f, boss.state_timer - dt);
    const float gap = distance(boss.position, player.position);
    const bool boar = world_.zone == Zone::BoarValley;
    const bool ogre = world_.zone == Zone::CloudPlateau;

    switch (boss.state) {
        case BossState::Dormant:
            if (gap < (ogre ? 20.0f : 17.0f)) {
                boss.state = BossState::Approach;
            }
            break;
        case BossState::Approach: {
            const float attack_gap = boar ? 4.3f : (ogre ? 5.0f : 3.0f);
            if (gap > attack_gap) {
                const Vec2 direction = normalized({player.position.x - boss.position.x,
                                                   player.position.z - boss.position.z});
                const float speed = boar ? 1.85f : (ogre ? 0.82f : 1.25f);
                boss.position.x += direction.x * speed * dt;
                boss.position.z += direction.z * speed * dt;
            } else {
                const unsigned cycle = boss.attack_cycle++ % (ogre ? 4U : 3U);
                if (ogre && cycle == 3U) {
                    boss.state = BossState::WindupMagic;
                    boss.state_timer = 1.55f;
                    boss.magic_target = player.position;
                } else {
                    const bool slam = cycle == 2U;
                    boss.state = slam ? BossState::WindupSlam : BossState::WindupSlash;
                    boss.state_timer = slam ? (ogre ? 1.40f : 1.10f)
                                            : (boar ? 0.95f : (ogre ? 1.05f : 0.80f));
                }
            }
            break;
        }
        case BossState::WindupSlash:
            if (boss.state_timer <= 0.0f) {
                boss.state = BossState::Slash;
                boss.state_timer = 0.18f;
                if (boar) {
                    boss.position = offset(boss.position, forwardOf(boss.facing), 2.4f);
                }
                const float reach = boar ? 2.0f : (ogre ? 2.2f : 1.45f);
                const Vec2 hit_center = offset(boss.position, forwardOf(boss.facing), reach);
                const float hit_radius = boar ? 1.65f : (ogre ? 1.75f : 1.20f);
                if (circlesOverlap(hit_center, hit_radius, player.position, 0.55f) &&
                    player.state != PlayerState::Dodge) {
                    PlayerController::damage(world_, boar ? 18.0f : (ogre ? 22.0f : 16.0f));
                }
            }
            break;
        case BossState::WindupSlam:
            if (boss.state_timer <= 0.0f) {
                boss.state = BossState::Slam;
                boss.state_timer = 0.25f;
                const float slam_radius = boar ? 3.1f : (ogre ? 4.25f : 2.75f);
                if (circlesOverlap(boss.position, slam_radius, player.position, 0.55f) &&
                    player.state != PlayerState::Dodge) {
                    PlayerController::damage(world_, ogre ? 26.0f : (boar ? 20.0f : 24.0f));
                }
            }
            break;
        case BossState::WindupMagic:
            if (boss.state_timer <= 0.0f) {
                boss.state = BossState::Magic;
                boss.state_timer = 0.42f;
                if (circlesOverlap(boss.magic_target, 2.6f, player.position, 0.55f) &&
                    player.state != PlayerState::Dodge) {
                    PlayerController::damage(world_, 24.0f);
                }
            }
            break;
        case BossState::Magic:
            if (boss.state_timer <= 0.0f) {
                boss.state = BossState::Recover;
                boss.state_timer = 1.15f;
            }
            break;
        case BossState::Slash:
        case BossState::Slam:
            if (boss.state_timer <= 0.0f) {
                boss.state = BossState::Recover;
                boss.state_timer = boar ? 1.05f : (ogre ? 1.20f : 0.90f);
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
        world_.victory_timer = 0.0f;
        if (world_.zone == Zone::BoarValley) {
            world_.boar_defeated = true;
            world_.player.health = std::min(100.0f, world_.player.health + 35.0f);
            world_.player.flasks = std::min(3, world_.player.flasks + 1);
        } else if (world_.zone == Zone::CloudPlateau) {
            world_.ogre_defeated = true;
            world_.player.health = std::min(100.0f, world_.player.health + 35.0f);
            world_.player.flasks = std::min(3, world_.player.flasks + 1);
        }
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
