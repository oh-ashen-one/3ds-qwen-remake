#pragma once

#include <3ds.h>

#include <cstdio>

namespace demake {

class AudioStreamer {
public:
    bool initialize();
    void update();
    void playHit(float pitch = 1.0f);
    void shutdown();

    unsigned underruns() const { return underruns_; }
    bool ambientAvailable() const { return ambient_file_ != nullptr; }

private:
    static constexpr int kSampleRate = 22050;
    static constexpr std::size_t kSamplesPerBuffer = 4096;
    static constexpr std::size_t kHitSamples = 1400;

    void fillAmbient(int index);

    bool ndsp_ready_ = false;
    std::FILE* ambient_file_ = nullptr;
    s16* ambient_samples_ = nullptr;
    s16* hit_samples_ = nullptr;
    ndspWaveBuf ambient_wave_[2]{};
    ndspWaveBuf hit_wave_{};
    unsigned underruns_ = 0;
};

} // namespace demake
