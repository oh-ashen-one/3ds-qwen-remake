#pragma once

#include <cstddef>
#include <cstdint>

namespace demake {

enum class Zone : std::uint8_t;

struct AssetRecord {
    const char* id;
    const char* kind;
    const char* output;
    std::uint32_t max_bytes;
    std::uint8_t zone_mask;
};

struct ZoneAssetRecord {
    const char* id;
    const char* display_name;
    std::uint32_t runtime_budget_bytes;
    std::uint32_t romfs_budget_bytes;
    std::uint16_t draw_call_budget;
    std::uint8_t asset_count;
};

class AssetRegistry {
public:
    static std::size_t assetCount();
    static const AssetRecord* assetAt(std::size_t index);
    static const AssetRecord* find(const char* id);
    static const ZoneAssetRecord& zone(Zone zone);
    static bool assetBelongsToZone(const AssetRecord& asset, Zone zone);
};

} // namespace demake
