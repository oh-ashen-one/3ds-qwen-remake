#include "demake/scene_assets.hpp"

#include "demake/generated/scene_asset_data.hpp"

namespace demake {

const SceneBox* SceneAssets::boxes(Zone zone, std::size_t& count) {
    switch (zone) {
        case Zone::Interior:
            count = generated::kInteriorBoxCount;
            return generated::kInteriorBoxes;
        case Zone::Vista:
            count = generated::kVistaBoxCount;
            return generated::kVistaBoxes;
        case Zone::Arena:
            count = generated::kArenaBoxCount;
            return generated::kArenaBoxes;
    }
    count = 0;
    return nullptr;
}

std::size_t SceneAssets::boxCount(Zone zone) {
    std::size_t count = 0;
    boxes(zone, count);
    return count;
}

} // namespace demake
