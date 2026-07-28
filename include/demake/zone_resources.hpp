#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "demake/scene_assets.hpp"

namespace demake {

class ZoneResources {
public:
    bool sync(std::uint8_t requested_mask);
    void shutdown();

    const SceneBox* boxes(Zone zone, std::size_t& count) const;
    std::uint32_t residentBytes() const { return resident_bytes_; }
    std::uint8_t loadedMask() const { return loaded_mask_; }
    unsigned loadCount() const { return load_count_; }
    unsigned unloadCount() const { return unload_count_; }

private:
    struct Slot {
        SceneBox* boxes = nullptr;
        std::size_t count = 0;
        std::uint32_t bytes = 0;
    };

    bool load(Zone zone);
    void unload(Zone zone);

    std::array<Slot, kZoneCount> slots_{};
    std::uint32_t resident_bytes_ = 0;
    std::uint8_t loaded_mask_ = 0;
    unsigned load_count_ = 0;
    unsigned unload_count_ = 0;
};

} // namespace demake
