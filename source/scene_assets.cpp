#include "demake/scene_assets.hpp"

#ifndef __3DS__
#include "demake/generated/scene_asset_data.hpp"
#endif

namespace demake {

const SceneBox* SceneAssets::boxes(Zone zone, std::size_t& count) {
#ifdef __3DS__
    (void)zone;
    count = 0;
    return nullptr;
#else
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
#endif
}

std::size_t SceneAssets::boxCount(Zone zone) {
    std::size_t count = 0;
    boxes(zone, count);
    return count;
}

} // namespace demake
