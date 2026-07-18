#include "demake/game_app.hpp"

#include <algorithm>
#include <cmath>

namespace demake {

bool GameApp::initialize() {
    if (R_FAILED(romfsInit())) {
        return false;
    }
    if (!renderer_.initialize()) {
        romfsExit();
        return false;
    }
    audio_.initialize();
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
    return input;
}

void GameApp::run() {
    u64 previous_ms = osGetTime();
    float accumulator = 0.0f;
    float frame_ms = 0.0f;
    float previous_boss_health = simulation_.world().boss.health;
    float previous_player_health = simulation_.world().player.health;

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
        if ((keys_down & KEY_START) != 0 && !title_screen_) {
            paused_ = !paused_;
        }

        InputFrame input = readInput(keys_down, keys_held);
        if (title_screen_) {
            if (input.interact) {
                simulation_.reset();
                title_screen_ = false;
                paused_ = false;
            }
        } else if (!paused_) {
            bool consumed_edges = false;
            while (accumulator >= kFixedStep) {
                InputFrame step_input = input;
                if (consumed_edges) {
                    step_input.light_attack = false;
                    step_input.heavy_attack = false;
                    step_input.dodge_pressed = false;
                    step_input.interact = false;
                    step_input.heal = false;
                    step_input.lock_toggle = false;
                    step_input.debug_toggle = false;
                }
                simulation_.step(step_input, kFixedStep);
                consumed_edges = true;
                accumulator -= kFixedStep;
            }
        }

        const WorldState& world = simulation_.world();
        if (world.boss.health < previous_boss_health) {
            audio_.playHit(1.2f);
        } else if (world.player.health < previous_player_health) {
            audio_.playHit(0.75f);
        }
        previous_boss_health = world.boss.health;
        previous_player_health = world.player.health;
        audio_.update();
        renderer_.render(world, title_screen_, paused_, frame_ms,
                         audio_.underruns(), audio_.ambientAvailable(), camera_yaw_);
    }
}

void GameApp::shutdown() {
    audio_.shutdown();
    renderer_.shutdown();
    romfsExit();
}

} // namespace demake
