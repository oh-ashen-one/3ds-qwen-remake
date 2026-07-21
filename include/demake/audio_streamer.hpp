#pragma once

#include "demake/core.hpp"

#include <3ds.h>

#include <cstdio>

namespace demake {

class AudioStreamer {
public:
    bool initialize();
    void setZone(Zone zone);
    void update();
    void playHit(float pitch = 1.0f);
    void suspend();
    void resume();
    void shutdown();

    unsigned underruns() const { return underruns_; }
    bool ambientAvailable() const { return music_file_ != nullptr; }

private:
    static constexpr int kSampleRate = 22050;
    static constexpr std::size_t kSamplesPerBuffer = 4096;
    static constexpr std::size_t kHitSamples = 1400;

    bool switchMusic(const char* path, bool boss_track);
    void configureMusicChannel();
    void fillMusic(int index);

    bool ndsp_ready_ = false;
    bool suspended_ = false;
    std::FILE* music_file_ = nullptr;
    s16* music_samples_ = nullptr;
    s16* hit_samples_ = nullptr;
    ndspWaveBuf music_wave_[2]{};
    ndspWaveBuf hit_wave_{};
    unsigned underruns_ = 0;
    bool boss_track_ = false;
};

} // namespace demake
