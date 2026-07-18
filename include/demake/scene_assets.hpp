#pragma once

#include <cstddef>
#include <cstdint>

#include "demake/core.hpp"

namespace demake {

struct SceneBox {
    float x;
    float y;
    float z;
    float sx;
    float sy;
    float sz;
    float rotation_y;
    float red;
    float green;
    float blue;
    std::int8_t cell_x;
    std::int8_t cell_z;
    bool always;
};

class SceneAssets {
public:
    static const SceneBox* boxes(Zone zone, std::size_t& count);
    static std::size_t boxCount(Zone zone);
};

} // namespace demake
