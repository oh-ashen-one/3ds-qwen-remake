#pragma once

#include "demake/core.hpp"
#include "demake/rigid_animation.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <tex3ds.h>

#include <array>
#include <cstddef>

namespace demake {

struct SceneBox;

class Renderer {
public:
    struct Vertex { float x; float y; float z; float u; float v; };

    bool initialize();
    void setHardwareInfo(const char* model_name);
    void render(const WorldState& world, bool title_screen, bool paused,
                float frame_ms, unsigned audio_underruns, bool audio_available,
                float camera_yaw, const SceneBox* scene_boxes,
                std::size_t scene_box_count, unsigned zone_resource_bytes,
                unsigned zone_memory_kb);
    void shutdown();

    unsigned drawCalls() const { return draw_calls_; }
    unsigned culledObjects() const { return culled_objects_; }

private:
    void bind3DState();
    void updateCamera(const WorldState& world);
    void renderWorld(const WorldState& world, const SceneBox* scene_boxes,
                     std::size_t scene_box_count);
    void renderPanorama(Zone zone);
    void renderStaticScene(const WorldState& world, const SceneBox* boxes, std::size_t count);
    void renderInterior(const WorldState& world);
    void renderVista(const WorldState& world);
    void renderArena(const WorldState& world);
    void renderField(const WorldState& world);
    void renderWildlife(const WorldState& world);
    void renderPlayer(const Player& player, const RigidPose& pose, float vertical_offset = 0.0f);
    void renderHumanoid(Vec2 position, float facing, float scale, const RigidPose& pose,
                        float red, float green, float blue, bool weapon,
                        float vertical_offset = 0.0f);
    void renderHorse(Vec2 position, float facing, float elapsed, bool moving, unsigned coat);
    void renderDeer(Vec2 position, float facing, float elapsed, float scale);
    void renderFox(Vec2 position, float facing, float elapsed);
    void renderCrane(Vec2 position, float facing, float elapsed);
    void renderBoss(const Boss& boss, float elapsed);
    void drawBlobShadow(Vec2 position, float scale);
    void drawBox(float x, float y, float z, float sx, float sy, float sz,
                 float rotation_y, float red, float green, float blue, bool always = false);
    void renderUi(const WorldState& world, bool title_screen, bool paused,
                  float frame_ms, unsigned audio_underruns, bool audio_available,
                  unsigned zone_resource_bytes, unsigned zone_memory_kb);
    void drawText(const char* value, float x, float y, float scale, u32 color,
                  float wrap_width = 0.0f);

    C3D_RenderTarget* top_target_ = nullptr;
    C3D_RenderTarget* bottom_target_ = nullptr;
    DVLB_s* vertex_shader_ = nullptr;
    shaderProgram_s program_{};
    int projection_location_ = -1;
    int model_view_location_ = -1;
    C3D_Mtx projection_{};
    C3D_Mtx view_{};
    C3D_AttrInfo attr_info_{};
    C3D_BufInfo buf_info_{};
    void* vbo_data_ = nullptr;
    void* index_data_ = nullptr;
    C3D_Tex environment_texture_{};
    Tex3DS_Texture environment_atlas_ = nullptr;
    C2D_TextBuf text_buffer_ = nullptr;
    Vec2 camera_ground_{};
    float camera_yaw_ = 0.0f;
    unsigned draw_calls_ = 0;
    unsigned visible_objects_ = 0;
    unsigned culled_objects_ = 0;
    const char* hardware_model_ = "Unknown 3DS";
    std::array<char, 256> keeper_dialogue_{};
};

} // namespace demake
