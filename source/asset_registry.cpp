#include "demake/asset_registry.hpp"

#include <cstring>

#include "demake/core.hpp"
#include "demake/generated/asset_registry_data.hpp"

namespace demake {

std::size_t AssetRegistry::assetCount() {
    return generated::kAssetCount;
}

const AssetRecord* AssetRegistry::assetAt(std::size_t index) {
    return index < generated::kAssetCount ? &generated::kAssets[index] : nullptr;
}

const AssetRecord* AssetRegistry::find(const char* id) {
    if (!id) {
        return nullptr;
    }
    for (std::size_t index = 0; index < generated::kAssetCount; ++index) {
        if (std::strcmp(generated::kAssets[index].id, id) == 0) {
            return &generated::kAssets[index];
        }
    }
    return nullptr;
}

const ZoneAssetRecord& AssetRegistry::zone(Zone zone_id) {
    std::size_t index = static_cast<std::size_t>(zone_id);
    if (index >= generated::kZoneCount) {
        index = 0;
    }
    return generated::kZones[index];
}

bool AssetRegistry::assetBelongsToZone(const AssetRecord& asset, Zone zone_id) {
    const unsigned bit = 1U << static_cast<unsigned>(zone_id);
    return (asset.zone_mask & bit) != 0;
}

} // namespace demake
