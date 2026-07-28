#pragma once

#include <array>
#include <cstdint>

namespace demake {

constexpr float kFixedStep = 1.0f / 30.0f;

struct Vec2 {
    float x = 0.0f;
    float z = 0.0f;
};

float length(Vec2 value);
float distance(Vec2 a, Vec2 b);
Vec2 normalized(Vec2 value);
bool circlesOverlap(Vec2 a, float radius_a, Vec2 b, float radius_b);
float sampleRigidSwing(float normalized_time);

enum class Zone : std::uint8_t {
    Interior,
    Vista,
    Arena,
    Field,
    BoarValley,
    CloudPlateau,
};

constexpr unsigned kZoneCount = 6;

enum class PlayerState : std::uint8_t {
    Idle,
    Move,
    Attack,
    HeavyAttack,
    Dodge,
    Hurt,
    Heal,
    Interact,
    Dead,
    Victory,
};

enum class BossState : std::uint8_t {
    Dormant,
    Approach,
    WindupSlash,
    Slash,
    WindupSlam,
    Slam,
    WindupMagic,
    Magic,
    Recover,
    Dead,
};

struct InputFrame {
    float move_x = 0.0f;
    float move_z = 0.0f;
    float camera_x = 0.0f;
    bool light_attack = false;
    bool heavy_attack = false;
    bool dodge_pressed = false;
    bool sprint_held = false;
    bool interact = false;
    bool heal = false;
    bool lock_toggle = false;
    bool debug_toggle = false;
    bool pause_toggle = false;
    int item_delta = 0;
};

constexpr int kMaximumFlasks = 5;
constexpr float kWardenMaximumHealth = 100.0f;

struct Player {
    Vec2 position{0.0f, -7.0f};
    float facing = 0.0f;
    float health = 100.0f;
    float stamina = 100.0f;
    int flasks = kMaximumFlasks;
    int selected_item = 0;
    PlayerState state = PlayerState::Idle;
    float state_timer = 0.0f;
    bool action_applied = false;
    bool lock_on = false;
    bool mounted = false;
};

const char* quickItemName(int selected_item);

struct Boss {
    Vec2 position{0.0f, 5.0f};
    Vec2 magic_target{};
    float facing = 3.14159265f;
    float health = kWardenMaximumHealth;
    BossState state = BossState::Dormant;
    float state_timer = 0.0f;
    unsigned attack_cycle = 0;
};

struct FieldHorse {
    Vec2 position{};
    float facing = 0.0f;
};

constexpr unsigned kFieldHorseCount = 3;

struct WorldState {
    Zone zone = Zone::Interior;
    Player player{};
    Boss boss{};
    float elapsed = 0.0f;
    float door_progress = 0.0f;
    float transition_timer = 0.0f;
    float arrival_fade_timer = 0.0f;
    float victory_timer = 0.0f;
    std::array<FieldHorse, kFieldHorseCount> horses{{
        {{2.0f, -28.0f}, 0.0f},
        {{-38.0f, 26.0f}, 1.15f},
        {{36.0f, 68.0f}, -0.80f},
    }};
    unsigned active_horse = 0;
    bool door_activated = false;
    bool dialogue_active = false;
    bool dialogue_complete = false;
    bool arena_transition = false;
    bool field_transition = false;
    bool branch_transition = false;
    Zone pending_zone = Zone::Interior;
    bool boar_defeated = false;
    bool ogre_defeated = false;
    bool debug_overlay = false;
    std::uint8_t loaded_zone_mask = 0;
    std::uint32_t zone_resident_bytes = 0;
    unsigned zone_loads = 0;
    unsigned zone_unloads = 0;
    unsigned zone_transitions = 0;
};

bool isBossZone(Zone zone);
float bossMaximumHealth(Zone zone);
const char* bossDisplayName(Zone zone);
float zoneGroundHeight(Zone zone, Vec2 position);

class ZoneManager {
public:
    void reset(WorldState& world) const;
    void update(WorldState& world, const InputFrame& input, float dt) const;
    void beginPostBossField(WorldState& world) const;
    static const char* name(Zone zone);
    static bool isLoaded(const WorldState& world, Zone zone);

private:
    static void beginBranchTransition(WorldState& world, Zone target);
    static void preload(WorldState& world, Zone zone);
    static void unload(WorldState& world, Zone zone);
    static void enter(WorldState& world, Zone zone);
};

class PlayerController {
public:
    void update(WorldState& world, const InputFrame& input, float dt) const;
    static void damage(WorldState& world, float amount);

private:
    static void finishTimedState(WorldState& world);
};

class BossController {
public:
    void update(WorldState& world, float dt) const;
    static void damage(WorldState& world, float amount);
};

class GameSimulation {
public:
    GameSimulation();

    void reset();
    void step(const InputFrame& input, float dt);
    const WorldState& world() const { return world_; }
    WorldState& mutableWorld() { return world_; }

private:
    void restartFromCheckpoint();

    WorldState world_{};
    ZoneManager zones_{};
    PlayerController player_controller_{};
    BossController boss_controller_{};
};

enum class SessionMode : std::uint8_t {
    Title,
    Playing,
    Paused,
    Suspended,
};

class GameSession {
public:
    void resetToTitle();
    void step(const InputFrame& input, float dt);
    void suspend();
    void resume();

    SessionMode mode() const { return mode_; }
    bool titleScreen() const { return mode_ == SessionMode::Title; }
    bool paused() const { return mode_ == SessionMode::Paused || mode_ == SessionMode::Suspended; }
    GameSimulation& simulation() { return simulation_; }
    const GameSimulation& simulation() const { return simulation_; }

private:
    GameSimulation simulation_{};
    SessionMode mode_ = SessionMode::Title;
    SessionMode mode_before_suspend_ = SessionMode::Playing;
};

} // namespace demake
