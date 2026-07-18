#include "demake/game_app.hpp"

#include <algorithm>
#include <cmath>

namespace demake {
namespace {

const char* modelName(u8 model) {
    switch (model) {
        case CFG_MODEL_3DS: return "Nintendo 3DS";
        case CFG_MODEL_3DSXL: return "Nintendo 3DS XL";
        case CFG_MODEL_N3DS: return "New Nintendo 3DS";
        case CFG_MODEL_2DS: return "Nintendo 2DS";
        case CFG_MODEL_N3DSXL: return "New Nintendo 3DS XL";
        case CFG_MODEL_N2DSXL: return "New Nintendo 2DS XL";
        default: return "Unknown 3DS";
    }
}

bool isNewFamily(u8 model) {
    return model == CFG_MODEL_N3DS || model == CFG_MODEL_N3DSXL || model == CFG_MODEL_N2DSXL;
}

} // namespace

bool GameApp::initialize() {
    if (R_FAILED(romfsInit())) {
        return false;
    }
    linear_baseline_ = linearSpaceFree();
    if (!renderer_.initialize()) {
        romfsExit();
        return false;
    }
    u8 system_model = 0xFF;
    if (R_SUCCEEDED(cfguInit())) {
        if (R_FAILED(CFGU_GetSystemModel(&system_model))) {
            system_model = 0xFF;
        }
        cfguExit();
    }
    renderer_.setHardwareInfo(modelName(system_model), isNewFamily(system_model));
    audio_.initialize();
    session_.resetToTitle();
    aptHook(&apt_hook_cookie_, &GameApp::aptEventHook, this);
    apt_hooked_ = true;
    running_ = true;
    return true;
}

InputFrame GameApp::readInput(u32 keys_down, u32 keys_held) {
    circlePosition circle{};
    circlePosition cstick{};
    hidCircleRead(&circle);
    hidCstickRead(&cstick);

    camera_yaw_ += static_cast<float>(cstick.dx) / 156.0f * 0.055f;
    const float raw_x = static_cast<float>(circle.dx) / 156.0f;
    const float raw_z = static_cast<float>(circle.dy) / 156.0f;
    const float cosine = std::cos(camera_yaw_);
    const float sine = std::sin(camera_yaw_);

    InputFrame input{};
    input.move_x = raw_x * cosine + raw_z * sine;
    input.move_z = raw_z * cosine - raw_x * sine;
    input.camera_x = static_cast<float>(cstick.dx) / 156.0f;
    input.light_attack = (keys_down & KEY_R) != 0;
    input.heavy_attack = (keys_down & KEY_ZR) != 0;
    input.dodge_pressed = (keys_down & KEY_B) != 0;
    input.sprint_held = (keys_held & KEY_B) != 0;
    input.interact = (keys_down & KEY_A) != 0;
    input.heal = (keys_down & KEY_X) != 0;
    input.lock_toggle = (keys_down & KEY_L) != 0;
    input.debug_toggle = (keys_down & KEY_Y) != 0;
    input.pause_toggle = (keys_down & KEY_START) != 0;
    if ((keys_down & (KEY_DLEFT | KEY_DDOWN)) != 0) {
        input.item_delta = -1;
    } else if ((keys_down & (KEY_DRIGHT | KEY_DUP)) != 0) {
        input.item_delta = 1;
    }
    return input;
}

void GameApp::latchEdges(const InputFrame& input) {
    pending_edges_.light_attack = pending_edges_.light_attack || input.light_attack;
    pending_edges_.heavy_attack = pending_edges_.heavy_attack || input.heavy_attack;
    pending_edges_.dodge_pressed = pending_edges_.dodge_pressed || input.dodge_pressed;
    pending_edges_.interact = pending_edges_.interact || input.interact;
    pending_edges_.heal = pending_edges_.heal || input.heal;
    pending_edges_.lock_toggle = pending_edges_.lock_toggle || input.lock_toggle;
    pending_edges_.debug_toggle = pending_edges_.debug_toggle || input.debug_toggle;
    pending_edges_.pause_toggle = pending_edges_.pause_toggle || input.pause_toggle;
    if (input.item_delta != 0) {
        pending_edges_.item_delta = input.item_delta;
    }
}

void GameApp::clearLatchedEdges() {
    pending_edges_.light_attack = false;
    pending_edges_.heavy_attack = false;
    pending_edges_.dodge_pressed = false;
    pending_edges_.interact = false;
    pending_edges_.heal = false;
    pending_edges_.lock_toggle = false;
    pending_edges_.debug_toggle = false;
    pending_edges_.pause_toggle = false;
    pending_edges_.item_delta = 0;
}

void GameApp::aptEventHook(APT_HookType hook, void* parameter) {
    GameApp* app = static_cast<GameApp*>(parameter);
    if (!app) {
        return;
    }
    if (hook == APTHOOK_ONSUSPEND || hook == APTHOOK_ONSLEEP) {
        app->session_.suspend();
        app->audio_.suspend();
    } else if (hook == APTHOOK_ONRESTORE || hook == APTHOOK_ONWAKEUP) {
        app->audio_.resume();
        app->session_.resume();
    } else if (hook == APTHOOK_ONEXIT) {
        app->running_ = false;
    }
}

void GameApp::run() {
    u64 previous_ms = osGetTime();
    float accumulator = 0.0f;
    float frame_ms = 0.0f;
    float previous_boss_health = session_.simulation().world().boss.health;
    float previous_player_health = session_.simulation().world().player.health;

    while (running_ && aptMainLoop()) {
        const u64 now_ms = osGetTime();
        frame_ms = static_cast<float>(now_ms - previous_ms);
        previous_ms = now_ms;
        accumulator = std::min(0.12f, accumulator + frame_ms / 1000.0f);

        hidScanInput();
        const u32 keys_down = hidKeysDown();
        const u32 keys_held = hidKeysHeld();
        if ((keys_down & KEY_SELECT) != 0) {
            running_ = false;
        }

        InputFrame input = readInput(keys_down, keys_held);
        latchEdges(input);
        bool consumed_edges = false;
        while (accumulator >= kFixedStep) {
            InputFrame step_input = input;
            if (!consumed_edges) {
                step_input.light_attack = pending_edges_.light_attack;
                step_input.heavy_attack = pending_edges_.heavy_attack;
                step_input.dodge_pressed = pending_edges_.dodge_pressed;
                step_input.interact = pending_edges_.interact;
                step_input.heal = pending_edges_.heal;
                step_input.lock_toggle = pending_edges_.lock_toggle;
                step_input.debug_toggle = pending_edges_.debug_toggle;
                step_input.pause_toggle = pending_edges_.pause_toggle;
                step_input.item_delta = pending_edges_.item_delta;
                clearLatchedEdges();
            } else {
                step_input.light_attack = false;
                step_input.heavy_attack = false;
                step_input.dodge_pressed = false;
                step_input.interact = false;
                step_input.heal = false;
                step_input.lock_toggle = false;
                step_input.debug_toggle = false;
                step_input.pause_toggle = false;
                step_input.item_delta = 0;
            }
            session_.step(step_input, kFixedStep);
            consumed_edges = true;
            accumulator -= kFixedStep;
        }

        const WorldState& world = session_.simulation().world();
        if (world.boss.health < previous_boss_health) {
            audio_.playHit(1.2f);
        } else if (world.player.health < previous_player_health) {
            audio_.playHit(0.75f);
        }
        previous_boss_health = world.boss.health;
        previous_player_health = world.player.health;
        audio_.update();
        const std::size_t current_free = linearSpaceFree();
        const std::size_t current_used = linear_baseline_ > current_free
                                             ? linear_baseline_ - current_free
                                             : 0;
        const unsigned zone_index = static_cast<unsigned>(world.zone);
        zone_peak_bytes_[zone_index] = std::max(zone_peak_bytes_[zone_index], current_used);
        renderer_.render(world, session_.titleScreen(), session_.paused(), frame_ms,
                         audio_.underruns(), audio_.ambientAvailable(), camera_yaw_,
                         static_cast<unsigned>(zone_peak_bytes_[zone_index] / 1024U));
    }
}

void GameApp::shutdown() {
    if (apt_hooked_) {
        aptUnhook(&apt_hook_cookie_);
        apt_hooked_ = false;
    }
    audio_.shutdown();
    renderer_.shutdown();
    romfsExit();
}

} // namespace demake
