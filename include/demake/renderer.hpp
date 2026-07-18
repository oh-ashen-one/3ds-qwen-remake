#pragma once

#include "demake/core.hpp"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

namespace demake {

class Renderer {
public:
    struct Vertex { float x; float y; float z; };

    bool initialize();
    void render(const WorldState& world, bool title_screen, bool paused,
                float frame_ms, unsigned audio_underruns, bool audio_available,
                float camera_yaw);
    void shutdown();

    unsigned drawCalls() const { return draw_calls_; }
    unsigned culledObjects() const { return culled_objects_; }

private:
    void bind3DState();
    void updateCamera(const WorldState& world);
    void renderWorld(const WorldState& world);
    void renderInterior(const WorldState& world);
    void renderVista(const WorldState& world);
    void renderArena(const WorldState& world);
    void renderHumanoid(Vec2 position, float facing, float scale, float swing,
                        float red, float green, float blue, bool weapon);
    void renderBoss(const Boss& boss, float elapsed);
    void drawBox(float x, float y, float z, float sx, float sy, float sz,
                 float rotation_y, float red, float green, float blue, bool always = false);
    void renderUi(const WorldState& world, bool title_screen, bool paused,
                  float frame_ms, unsigned audio_underruns, bool audio_available);
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
    void* vbo_data_ = nullptr;
    C2D_TextBuf text_buffer_ = nullptr;
    Vec2 camera_ground_{};
    float camera_yaw_ = 0.0f;
    unsigned draw_calls_ = 0;
    unsigned culled_objects_ = 0;
};

} // namespace demake
