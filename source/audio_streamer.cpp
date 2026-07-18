#include "demake/audio_streamer.hpp"

#include <cmath>
#include <cstring>

namespace demake {

bool AudioStreamer::initialize() {
    if (R_FAILED(ndspInit())) {
        return false;
    }
    ndsp_ready_ = true;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    ambient_samples_ = static_cast<s16*>(linearAlloc(kSamplesPerBuffer * 2U * sizeof(s16)));
    hit_samples_ = static_cast<s16*>(linearAlloc(kHitSamples * sizeof(s16)));
    if (!ambient_samples_ || !hit_samples_) {
        shutdown();
        return false;
    }

    ambient_file_ = std::fopen("romfs:/audio/ambient.pcm", "rb");
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, static_cast<float>(kSampleRate));
    ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
    float ambient_mix[12]{};
    ambient_mix[0] = 0.38f;
    ambient_mix[1] = 0.38f;
    ndspChnSetMix(0, ambient_mix);

    if (ambient_file_) {
        for (int index = 0; index < 2; ++index) {
            ambient_wave_[index].data_vaddr = ambient_samples_ + index * kSamplesPerBuffer;
            fillAmbient(index);
            ndspChnWaveBufAdd(0, &ambient_wave_[index]);
        }
    }

    for (std::size_t i = 0; i < kHitSamples; ++i) {
        const float time = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float envelope = 1.0f - static_cast<float>(i) / static_cast<float>(kHitSamples);
        hit_samples_[i] = static_cast<s16>(std::sin(time * 1100.0f * 6.2831853f) *
                                           envelope * envelope * 9000.0f);
    }
    DSP_FlushDataCache(hit_samples_, kHitSamples * sizeof(s16));
    ndspChnSetInterp(1, NDSP_INTERP_LINEAR);
    ndspChnSetRate(1, static_cast<float>(kSampleRate));
    ndspChnSetFormat(1, NDSP_FORMAT_MONO_PCM16);
    float hit_mix[12]{};
    hit_mix[0] = 0.65f;
    hit_mix[1] = 0.65f;
    ndspChnSetMix(1, hit_mix);
    return true;
}

void AudioStreamer::fillAmbient(int index) {
    if (!ambient_file_ || !ambient_samples_) {
        return;
    }
    s16* destination = ambient_samples_ + index * kSamplesPerBuffer;
    std::size_t written = 0;
    unsigned empty_reads = 0;
    while (written < kSamplesPerBuffer) {
        const std::size_t count = std::fread(destination + written, sizeof(s16),
                                             kSamplesPerBuffer - written, ambient_file_);
        written += count;
        if (count == 0) {
            ++empty_reads;
            std::rewind(ambient_file_);
            if (std::ferror(ambient_file_) || empty_reads >= 2) {
                std::clearerr(ambient_file_);
                break;
            }
        } else {
            empty_reads = 0;
        }
    }
    if (written < kSamplesPerBuffer) {
        std::memset(destination + written, 0, (kSamplesPerBuffer - written) * sizeof(s16));
    }
    DSP_FlushDataCache(destination, kSamplesPerBuffer * sizeof(s16));
    ambient_wave_[index].nsamples = kSamplesPerBuffer;
}

void AudioStreamer::update() {
    if (!ndsp_ready_ || !ambient_file_) {
        return;
    }
    bool any_queued = false;
    for (int index = 0; index < 2; ++index) {
        if (ambient_wave_[index].status == NDSP_WBUF_DONE) {
            fillAmbient(index);
            ndspChnWaveBufAdd(0, &ambient_wave_[index]);
        } else {
            any_queued = true;
        }
    }
    if (!any_queued) {
        ++underruns_;
    }
}

void AudioStreamer::playHit(float pitch) {
    if (!ndsp_ready_ || !hit_samples_) {
        return;
    }
    if (hit_wave_.status == NDSP_WBUF_QUEUED || hit_wave_.status == NDSP_WBUF_PLAYING) {
        return;
    }
    ndspChnSetRate(1, static_cast<float>(kSampleRate) * pitch);
    std::memset(&hit_wave_, 0, sizeof(hit_wave_));
    hit_wave_.data_vaddr = hit_samples_;
    hit_wave_.nsamples = kHitSamples;
    ndspChnWaveBufAdd(1, &hit_wave_);
}

void AudioStreamer::shutdown() {
    if (ambient_file_) {
        std::fclose(ambient_file_);
        ambient_file_ = nullptr;
    }
    if (ndsp_ready_) {
        ndspChnReset(0);
        ndspChnReset(1);
        ndspExit();
        ndsp_ready_ = false;
    }
    if (ambient_samples_) {
        linearFree(ambient_samples_);
        ambient_samples_ = nullptr;
    }
    if (hit_samples_) {
        linearFree(hit_samples_);
        hit_samples_ = nullptr;
    }
}

} // namespace demake
