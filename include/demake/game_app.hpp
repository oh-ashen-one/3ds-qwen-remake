#pragma once

#include "demake/audio_streamer.hpp"
#include "demake/core.hpp"
#include "demake/renderer.hpp"
#include "demake/zone_resources.hpp"

#include <cstddef>

namespace demake {

class GameApp {
public:
    bool initialize();
    void run();
    void shutdown();

private:
    InputFrame readInput(u32 keys_down, u32 keys_held);
    void latchEdges(const InputFrame& input);
    void clearLatchedEdges();
    static void aptEventHook(APT_HookType hook, void* parameter);

    GameSession session_{};
    Renderer renderer_{};
    ZoneResources zone_resources_{};
    AudioStreamer audio_{};
    InputFrame pending_edges_{};
    aptHookCookie apt_hook_cookie_{};
    bool running_ = false;
    bool apt_hooked_ = false;
    bool new_family_hardware_ = false;
    float camera_yaw_ = 0.0f;
    std::size_t linear_baseline_ = 0;
    std::size_t zone_peak_bytes_[kZoneCount]{};
};

} // namespace demake
