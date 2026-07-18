#pragma once

#include "demake/audio_streamer.hpp"
#include "demake/core.hpp"
#include "demake/renderer.hpp"

namespace demake {

class GameApp {
public:
    bool initialize();
    void run();
    void shutdown();

private:
    InputFrame readInput(u32 keys_down, u32 keys_held);

    GameSimulation simulation_{};
    Renderer renderer_{};
    AudioStreamer audio_{};
    bool running_ = false;
    bool title_screen_ = true;
    bool paused_ = false;
    float camera_yaw_ = 0.0f;
};

} // namespace demake
