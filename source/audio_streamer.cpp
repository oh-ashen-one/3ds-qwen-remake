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

    music_samples_ = static_cast<s16*>(linearAlloc(kSamplesPerBuffer * 2U * sizeof(s16)));
    hit_samples_ = static_cast<s16*>(linearAlloc(kHitSamples * sizeof(s16)));
    if (!music_samples_ || !hit_samples_) {
        shutdown();
        return false;
    }

    if (!switchMusic("romfs:/audio/ashen_deep_hall.pcm", MusicTrack::DeepHall)) {
        shutdown();
        return false;
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

void AudioStreamer::configureMusicChannel() {
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, static_cast<float>(kSampleRate));
    ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
    float music_mix[12]{};
    const float volume = music_track_ == MusicTrack::AshenGate ? 0.32f : 0.24f;
    music_mix[0] = volume;
    music_mix[1] = volume;
    ndspChnSetMix(0, music_mix);
}

bool AudioStreamer::switchMusic(const char* path, MusicTrack track) {
    std::FILE* next_file = std::fopen(path, "rb");
    if (!next_file) {
        return false;
    }
    ndspChnReset(0);
    if (music_file_) {
        std::fclose(music_file_);
    }
    music_file_ = next_file;
    music_track_ = track;
    std::memset(music_wave_, 0, sizeof(music_wave_));
    configureMusicChannel();
    for (int index = 0; index < 2; ++index) {
        music_wave_[index].data_vaddr = music_samples_ + index * kSamplesPerBuffer;
        fillMusic(index);
        ndspChnWaveBufAdd(0, &music_wave_[index]);
    }
    return true;
}

void AudioStreamer::setZone(Zone zone) {
    const MusicTrack wanted_track =
        zone == Zone::Arena ? MusicTrack::AshenGate
                            : (zone == Zone::Field ? MusicTrack::ValleyAfterDawn
                                                  : MusicTrack::DeepHall);
    if (wanted_track == music_track_) {
        return;
    }
    const char* path = wanted_track == MusicTrack::AshenGate
                           ? "romfs:/audio/ashen_gate.pcm"
                           : (wanted_track == MusicTrack::ValleyAfterDawn
                                  ? "romfs:/audio/valley_after_dawn.pcm"
                                  : "romfs:/audio/ashen_deep_hall.pcm");
    switchMusic(path, wanted_track);
}

void AudioStreamer::fillMusic(int index) {
    if (!music_file_ || !music_samples_) {
        return;
    }
    s16* destination = music_samples_ + index * kSamplesPerBuffer;
    std::size_t written = 0;
    unsigned empty_reads = 0;
    while (written < kSamplesPerBuffer) {
        const std::size_t count = std::fread(destination + written, sizeof(s16),
                                             kSamplesPerBuffer - written, music_file_);
        written += count;
        if (count == 0) {
            ++empty_reads;
            std::rewind(music_file_);
            if (std::ferror(music_file_) || empty_reads >= 2) {
                std::clearerr(music_file_);
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
    music_wave_[index].nsamples = kSamplesPerBuffer;
}

void AudioStreamer::update() {
    if (!ndsp_ready_ || !music_file_) {
        return;
    }
    bool any_queued = false;
    for (int index = 0; index < 2; ++index) {
        if (music_wave_[index].status == NDSP_WBUF_DONE) {
            fillMusic(index);
            ndspChnWaveBufAdd(0, &music_wave_[index]);
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

void AudioStreamer::suspend() {
    if (!ndsp_ready_ || suspended_) {
        return;
    }
    ndspChnSetPaused(0, true);
    ndspChnSetPaused(1, true);
    suspended_ = true;
}

void AudioStreamer::resume() {
    if (!ndsp_ready_ || !suspended_) {
        return;
    }
    ndspChnSetPaused(0, false);
    ndspChnSetPaused(1, false);
    suspended_ = false;
}

void AudioStreamer::shutdown() {
    if (music_file_) {
        std::fclose(music_file_);
        music_file_ = nullptr;
    }
    if (ndsp_ready_) {
        ndspChnReset(0);
        ndspChnReset(1);
        ndspExit();
        ndsp_ready_ = false;
        suspended_ = false;
    }
    if (music_samples_) {
        linearFree(music_samples_);
        music_samples_ = nullptr;
    }
    if (hit_samples_) {
        linearFree(hit_samples_);
        hit_samples_ = nullptr;
    }
}

} // namespace demake
