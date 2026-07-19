#include "demake/renderer.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "demake/scene_assets.hpp"
#include "environment_atlas_t3x.h"
#include "vshader_shbin.h"

namespace demake {
namespace {

constexpr u32 kTransferFlags =
    GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);

constexpr Renderer::Vertex kCube[] = {
    {-0.5f,-0.5f, 0.5f,0.0f,0.0f},{ 0.5f,-0.5f, 0.5f,1.0f,0.0f},
    { 0.5f, 0.5f, 0.5f,1.0f,1.0f},{-0.5f, 0.5f, 0.5f,0.0f,1.0f},
    { 0.5f,-0.5f,-0.5f,0.0f,0.0f},{-0.5f,-0.5f,-0.5f,1.0f,0.0f},
    {-0.5f, 0.5f,-0.5f,1.0f,1.0f},{ 0.5f, 0.5f,-0.5f,0.0f,1.0f},
    { 0.5f,-0.5f, 0.5f,0.0f,0.0f},{ 0.5f,-0.5f,-0.5f,1.0f,0.0f},
    { 0.5f, 0.5f,-0.5f,1.0f,1.0f},{ 0.5f, 0.5f, 0.5f,0.0f,1.0f},
    {-0.5f,-0.5f,-0.5f,0.0f,0.0f},{-0.5f,-0.5f, 0.5f,1.0f,0.0f},
    {-0.5f, 0.5f, 0.5f,1.0f,1.0f},{-0.5f, 0.5f,-0.5f,0.0f,1.0f},
    {-0.5f, 0.5f, 0.5f,0.0f,0.0f},{ 0.5f, 0.5f, 0.5f,1.0f,0.0f},
    { 0.5f, 0.5f,-0.5f,1.0f,1.0f},{-0.5f, 0.5f,-0.5f,0.0f,1.0f},
    {-0.5f,-0.5f,-0.5f,0.0f,0.0f},{ 0.5f,-0.5f,-0.5f,1.0f,0.0f},
    { 0.5f,-0.5f, 0.5f,1.0f,1.0f},{-0.5f,-0.5f, 0.5f,0.0f,1.0f},
};

constexpr std::uint8_t kCubeIndices[] = {
    0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4,
    8, 9, 10, 10, 11, 8, 12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20,
};

constexpr std::size_t kCubeIndexCount = sizeof(kCubeIndices) / sizeof(kCubeIndices[0]);

bool loadTextAsset(const char* path, char* destination, std::size_t capacity) {
    if (!path || !destination || capacity < 2) {
        return false;
    }
    std::FILE* file = std::fopen(path, "rb");
    if (!file) {
        return false;
    }
    const std::size_t bytes = std::fread(destination, 1, capacity - 1, file);
    const bool valid = bytes > 0 && !std::ferror(file) && std::fgetc(file) == EOF;
    std::fclose(file);
    if (!valid) {
        destination[0] = '\0';
        return false;
    }
    destination[bytes] = '\0';
    return true;
}

} // namespace

bool Renderer::initialize() {
    gfxInitDefault();
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return false;
    }
    if (!C2D_Init(2048)) {
        C3D_Fini();
        gfxExit();
        return false;
    }
    C2D_Prepare();
    top_target_ = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom_target_ = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    C3D_RenderTargetSetOutput(top_target_, GFX_TOP, GFX_LEFT, kTransferFlags);
    C3D_RenderTargetSetOutput(bottom_target_, GFX_BOTTOM, GFX_LEFT, kTransferFlags);

    vertex_shader_ = DVLB_ParseFile(
        reinterpret_cast<u32*>(const_cast<std::uint8_t*>(vshader_shbin)),
        vshader_shbin_size);
    if (!vertex_shader_) {
        shutdown();
        return false;
    }
    shaderProgramInit(&program_);
    shaderProgramSetVsh(&program_, &vertex_shader_->DVLE[0]);
    projection_location_ = shaderInstanceGetUniformLocation(program_.vertexShader, "projection");
    model_view_location_ = shaderInstanceGetUniformLocation(program_.vertexShader, "modelView");
    Mtx_PerspTilt(&projection_, C3D_AngleFromDegrees(55.0f), C3D_AspectRatioTop,
                  0.1f, 130.0f, false);

    vbo_data_ = linearAlloc(sizeof(kCube));
    if (!vbo_data_) {
        shutdown();
        return false;
    }
    std::memcpy(vbo_data_, kCube, sizeof(kCube));
    index_data_ = linearAlloc(sizeof(kCubeIndices));
    if (!index_data_) {
        shutdown();
        return false;
    }
    std::memcpy(index_data_, kCubeIndices, sizeof(kCubeIndices));
    AttrInfo_Init(&attr_info_);
    AttrInfo_AddLoader(&attr_info_, 0, GPU_FLOAT, 3);
    AttrInfo_AddFixed(&attr_info_, 1);
    AttrInfo_AddLoader(&attr_info_, 2, GPU_FLOAT, 2);
    BufInfo_Init(&buf_info_);
    BufInfo_Add(&buf_info_, vbo_data_, sizeof(Vertex), 2, 0x20);
    environment_atlas_ = Tex3DS_TextureImport(
        environment_atlas_t3x, environment_atlas_t3x_size,
        &environment_texture_, nullptr, false);
    if (!environment_atlas_) {
        shutdown();
        return false;
    }
    C3D_TexSetFilter(&environment_texture_, GPU_LINEAR, GPU_NEAREST);
    C3D_TexSetWrap(&environment_texture_, GPU_REPEAT, GPU_REPEAT);
    text_buffer_ = C2D_TextBufNew(4096);
    if (!text_buffer_) {
        shutdown();
        return false;
    }
    if (!loadTextAsset("romfs:/dialogue/keeper.txt", keeper_dialogue_.data(),
                       keeper_dialogue_.size())) {
        shutdown();
        return false;
    }
    return true;
}

void Renderer::setHardwareInfo(const char* model_name) {
    hardware_model_ = model_name ? model_name : "Unknown 3DS";
}

void Renderer::bind3DState() {
    C3D_BindProgram(&program_);
    C3D_SetAttrInfo(&attr_info_);
    C3D_SetBufInfo(&buf_info_);
    C3D_TexBind(0, &environment_texture_);
    C3D_TexEnv* environment = C3D_GetTexEnv(0);
    C3D_TexEnvInit(environment);
    C3D_TexEnvSrc(environment, C3D_Both,
                  GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(environment, C3D_Both, GPU_MODULATE);
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_BACK_CCW);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, projection_location_, &projection_);
}

void Renderer::updateCamera(const WorldState& world) {
    const Player& player = world.player;
    if (player.lock_on && world.zone == Zone::Arena && world.boss.state != BossState::Dead) {
        camera_yaw_ = std::atan2(world.boss.position.x - player.position.x,
                                 world.boss.position.z - player.position.z);
    } else {
        camera_yaw_ += 0.0f;
    }
    const float forward_x = std::sin(camera_yaw_);
    const float forward_z = std::cos(camera_yaw_);
    camera_ground_ = {player.position.x - forward_x * 7.2f,
                      player.position.z - forward_z * 7.2f};
    const C3D_FVec camera = FVec3_New(camera_ground_.x, 4.5f, camera_ground_.z);
    const C3D_FVec target = FVec3_New(player.position.x, 1.1f, player.position.z + 1.2f);
    const C3D_FVec up = FVec3_New(0.0f, 1.0f, 0.0f);
    Mtx_LookAt(&view_, camera, target, up, false);
}

void Renderer::render(const WorldState& world, bool title_screen, bool paused,
                      float frame_ms, unsigned audio_underruns, bool audio_available,
                      float camera_yaw, const SceneBox* scene_boxes,
                      std::size_t scene_box_count, unsigned zone_resource_bytes,
                      unsigned zone_memory_kb) {
    draw_calls_ = 0;
    visible_objects_ = 0;
    culled_objects_ = 0;
    if (!world.player.lock_on) {
        camera_yaw_ = camera_yaw;
    }
    updateCamera(world);

    const u32 clear_color = world.zone == Zone::Interior ? 0x17101DFF : 0x6B5B51FF;
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_RenderTargetClear(top_target_, C3D_CLEAR_ALL, clear_color, 0);
    C3D_FrameDrawOn(top_target_);
    bind3DState();
    renderWorld(world, scene_boxes, scene_box_count);
    renderUi(world, title_screen, paused, frame_ms, audio_underruns, audio_available,
             zone_resource_bytes, zone_memory_kb);
    C3D_FrameEnd(0);
}

void Renderer::renderWorld(const WorldState& world, const SceneBox* scene_boxes,
                           std::size_t scene_box_count) {
    renderPanorama(world.zone);
    renderStaticScene(scene_boxes, scene_box_count);
    switch (world.zone) {
        case Zone::Interior: renderInterior(world); break;
        case Zone::Vista: renderVista(world); break;
        case Zone::Arena: renderArena(world); break;
    }
    RigidPose player_pose{};
    samplePlayerPose(world.player, world.elapsed, player_pose);
    renderHumanoid(world.player.position, world.player.facing, 1.0f,
                   player_pose, 0.42f, 0.48f, 0.55f, true);
}

void Renderer::renderPanorama(Zone zone) {
    if (zone == Zone::Interior) {
        return;
    }
    const float red = zone == Zone::Vista ? 0.34f : 0.22f;
    const float green = zone == Zone::Vista ? 0.28f : 0.18f;
    const float blue = zone == Zone::Vista ? 0.36f : 0.20f;
    drawBox(0.0f, 15.0f, 78.0f, 120.0f, 30.0f, 1.0f,
            0.0f, red, green, blue, true);
    drawBox(-58.0f, 13.0f, 34.0f, 1.0f, 26.0f, 88.0f,
            0.0f, red * 0.72f, green * 0.72f, blue * 0.82f, true);
    drawBox(58.0f, 13.0f, 34.0f, 1.0f, 26.0f, 88.0f,
            0.0f, red * 0.78f, green * 0.78f, blue * 0.88f, true);
}

void Renderer::renderStaticScene(const SceneBox* boxes, std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        const SceneBox& box = boxes[index];
        if (!box.always) {
            const Vec2 cell_center{
                static_cast<float>(box.cell_x) * 6.0f + 3.0f,
                static_cast<float>(box.cell_z) * 6.0f + 3.0f,
            };
            if (distance(camera_ground_, cell_center) > 62.0f) {
                ++culled_objects_;
                continue;
            }
        }
        drawBox(box.x, box.y, box.z, box.sx, box.sy, box.sz, box.rotation_y,
                box.red, box.green, box.blue, box.always);
    }
}

void Renderer::renderInterior(const WorldState& world) {
    const float door_y = 2.0f + world.door_progress * 4.2f;
    drawBox(0.0f, door_y, 4.6f, 4.2f, 4.0f, 0.5f, 0.0f, 0.35f, 0.30f, 0.25f, true);
}

void Renderer::renderVista(const WorldState& world) {
    Player keeper{};
    keeper.state = PlayerState::Idle;
    RigidPose keeper_pose{};
    samplePlayerPose(keeper, world.elapsed, keeper_pose);
    renderHumanoid({0.0f, 15.5f}, 3.14159265f, 0.95f,
                   keeper_pose, 0.25f, 0.22f, 0.40f, false);
    const float fog_pulse = 0.55f + std::sin(world.elapsed * 2.0f) * 0.08f;
    drawBox(0.0f, 2.0f, 28.4f, 7.0f, 4.0f, 0.25f, 0.0f,
            fog_pulse, fog_pulse, fog_pulse * 0.75f, true);
}

void Renderer::renderArena(const WorldState& world) {
    renderBoss(world.boss, world.elapsed);
}

void Renderer::renderHumanoid(Vec2 position, float facing, float scale, const RigidPose& pose,
                              float red, float green, float blue, bool weapon) {
    drawBlobShadow(position, scale);
    const float side_x = std::cos(facing);
    const float side_z = -std::sin(facing);
    const float forward_x = std::sin(facing);
    const float forward_z = std::cos(facing);
    const float root_y = pose.at(Bone::Root).vertical * scale;
    drawBox(position.x, 0.98f * scale + root_y, position.z,
            0.72f * scale, 0.34f * scale, 0.44f * scale,
            facing + pose.at(Bone::Pelvis).yaw, red * 0.78f, green * 0.78f, blue * 0.82f);
    drawBox(position.x + forward_x * pose.at(Bone::Torso).forward * scale,
            1.48f * scale + root_y, position.z + forward_z * pose.at(Bone::Torso).forward * scale,
            0.75f * scale, 0.78f * scale, 0.45f * scale,
            facing + pose.at(Bone::Torso).yaw, red, green, blue);
    drawBox(position.x + forward_x * pose.at(Bone::Head).forward * scale,
            2.05f * scale + root_y + pose.at(Bone::Head).vertical * scale,
            position.z + forward_z * pose.at(Bone::Head).forward * scale,
            0.48f * scale, 0.52f * scale,
            0.48f * scale, facing, red * 0.85f, green * 0.78f, blue * 0.72f);
    for (int side = -1; side <= 1; side += 2) {
        const Bone upper_leg = side < 0 ? Bone::LeftUpperLeg : Bone::RightUpperLeg;
        const Bone lower_leg = side < 0 ? Bone::LeftLowerLeg : Bone::RightLowerLeg;
        const Bone foot = side < 0 ? Bone::LeftFoot : Bone::RightFoot;
        const Bone upper_arm = side < 0 ? Bone::LeftUpperArm : Bone::RightUpperArm;
        const Bone lower_arm = side < 0 ? Bone::LeftLowerArm : Bone::RightLowerArm;
        const float leg_stride = pose.at(upper_leg).forward;
        drawBox(position.x + side_x * side * 0.22f * scale + forward_x * leg_stride,
                0.65f * scale + root_y,
                position.z + side_z * side * 0.22f * scale + forward_z * leg_stride,
                0.24f * scale, 0.48f * scale, 0.27f * scale,
                facing + pose.at(upper_leg).yaw, red * 0.72f, green * 0.72f, blue * 0.77f);
        drawBox(position.x + side_x * side * 0.22f * scale + forward_x * pose.at(lower_leg).forward,
                0.25f * scale + root_y,
                position.z + side_z * side * 0.22f * scale + forward_z * pose.at(lower_leg).forward,
                0.21f * scale, 0.40f * scale, 0.23f * scale,
                facing + pose.at(lower_leg).yaw, red * 0.66f, green * 0.66f, blue * 0.72f);
        drawBox(position.x + side_x * side * 0.22f * scale +
                    forward_x * (0.10f + pose.at(foot).forward) * scale,
                0.05f * scale + root_y,
                position.z + side_z * side * 0.22f * scale +
                    forward_z * (0.10f + pose.at(foot).forward) * scale,
                0.24f * scale, 0.12f * scale, 0.42f * scale,
                facing + pose.at(foot).yaw, red * 0.58f, green * 0.58f, blue * 0.64f);

        const float arm_reach = pose.at(upper_arm).forward;
        drawBox(position.x + side_x * side * 0.53f * scale + forward_x * arm_reach,
                1.52f * scale + root_y,
                position.z + side_z * side * 0.53f * scale + forward_z * arm_reach,
                0.23f * scale, 0.44f * scale, 0.24f * scale,
                facing + pose.at(upper_arm).yaw, red * 0.76f, green * 0.76f, blue * 0.81f);
        drawBox(position.x + side_x * side * 0.58f * scale +
                    forward_x * pose.at(lower_arm).forward,
                1.16f * scale + root_y,
                position.z + side_z * side * 0.58f * scale +
                    forward_z * pose.at(lower_arm).forward,
                0.20f * scale, 0.40f * scale, 0.21f * scale,
                facing + pose.at(lower_arm).yaw, red * 0.68f, green * 0.68f, blue * 0.75f);
        drawBox(position.x + side_x * side * 0.60f * scale + forward_x * arm_reach * 1.2f,
                0.91f * scale + root_y,
                position.z + side_z * side * 0.60f * scale + forward_z * arm_reach * 1.2f,
                0.18f * scale, 0.20f * scale, 0.18f * scale,
                facing, red * 0.82f, green * 0.72f, blue * 0.68f);
    }
    if (weapon) {
        const BoneTransform& weapon_pose = pose.at(Bone::Weapon);
        drawBox(position.x + side_x * 0.72f * scale +
                    forward_x * (0.8f + weapon_pose.forward) * scale,
                1.25f * scale + root_y,
                position.z + side_z * 0.72f * scale +
                    forward_z * (0.8f + weapon_pose.forward) * scale,
                0.10f * scale, 0.12f * scale, 1.85f * scale,
                facing + weapon_pose.yaw, 0.72f, 0.74f, 0.78f);
    }
}

void Renderer::drawBlobShadow(Vec2 position, float scale) {
    drawBox(position.x, 0.012f, position.z,
            1.15f * scale, 0.025f, 0.72f * scale,
            0.0f, 0.055f, 0.045f, 0.055f, true);
}

void Renderer::renderBoss(const Boss& boss, float elapsed) {
    if (boss.state == BossState::Dead) {
        drawBox(boss.position.x, 0.25f, boss.position.z, 2.8f, 0.45f, 1.2f,
                boss.facing + 1.57f, 0.35f, 0.12f, 0.10f, true);
        return;
    }
    RigidPose boss_pose{};
    sampleBossPose(boss, elapsed, boss_pose);
    renderHumanoid(boss.position, boss.facing, 1.75f,
                   boss_pose,
                   0.46f, 0.16f, 0.12f, true);
    const float side_x = std::cos(boss.facing);
    const float side_z = -std::sin(boss.facing);
    for (int side = -1; side <= 1; side += 2) {
        drawBox(boss.position.x + side_x * side * 0.30f, 4.05f,
                boss.position.z + side_z * side * 0.30f,
                0.18f, 0.75f, 0.18f, boss.facing + side * 0.35f,
                0.70f, 0.55f, 0.32f);
    }
}

void Renderer::drawBox(float x, float y, float z, float sx, float sy, float sz,
                       float rotation_y, float red, float green, float blue, bool always) {
    if (!always) {
        const Vec2 to_object{x - camera_ground_.x, z - camera_ground_.z};
        const float object_distance = length(to_object);
        const float forward_dot = object_distance > 0.001f
                                      ? (to_object.x * std::sin(camera_yaw_) +
                                         to_object.z * std::cos(camera_yaw_)) / object_distance
                                      : 1.0f;
        if (object_distance > 52.0f || (object_distance > 6.0f && forward_dot < 0.12f)) {
            ++culled_objects_;
            return;
        }
    }
    C3D_Mtx model;
    C3D_Mtx model_view;
    Mtx_Identity(&model);
    Mtx_Translate(&model, x, y, z, true);
    Mtx_RotateY(&model, rotation_y, true);
    Mtx_Scale(&model, sx, sy, sz);
    Mtx_Multiply(&model_view, &view_, &model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, model_view_location_, &model_view);
    // Original asset colors provide baked lighting; one cheap directional term
    // adds consistent outdoor shape without a per-pixel lighting pass.
    const float directional = 0.80f +
                              0.18f * std::fmax(0.0f, std::cos(rotation_y - 0.65f));
    C3D_FixedAttribSet(1, red * directional, green * directional,
                      blue * directional, 1.0f);
    C3D_DrawElements(GPU_TRIANGLES, kCubeIndexCount, C3D_UNSIGNED_BYTE, index_data_);
    ++draw_calls_;
    ++visible_objects_;
}

void Renderer::drawText(const char* value, float x, float y, float scale, u32 color,
                        float wrap_width) {
    C2D_Text text;
    C2D_TextParse(&text, text_buffer_, value);
    C2D_TextOptimize(&text);
    if (wrap_width > 0.0f) {
        C2D_DrawText(&text, C2D_WithColor | C2D_WordWrap, x, y, 0.5f,
                     scale, scale, color, wrap_width);
    } else {
        C2D_DrawText(&text, C2D_WithColor, x, y, 0.5f, scale, scale, color);
    }
}

void Renderer::renderUi(const WorldState& world, bool title_screen, bool paused,
                        float frame_ms, unsigned audio_underruns, bool audio_available,
                        unsigned zone_resource_bytes, unsigned zone_memory_kb) {
    // Raw citro3d world rendering replaces citro2d's shader and vertex state.
    // Restore it before queuing either screen's overlay batches.
    C2D_Prepare();
    C2D_TextBufClear(text_buffer_);
    C2D_SceneBegin(top_target_);
    if (title_screen) {
        C2D_DrawRectSolid(0.0f, 0.0f, 0.2f, 400.0f, 240.0f, C2D_Color32(8, 7, 11, 205));
        drawText("ASHEN RIFT", 200.0f, 52.0f, 1.05f, C2D_Color32(222, 188, 112, 255));
        drawText("AN ORIGINAL NINTENDO 3DS TALE", 83.0f, 92.0f, 0.42f,
                 C2D_Color32(205, 205, 210, 255));
        drawText("Press A to descend", 126.0f, 168.0f, 0.52f, C2D_Color32(245, 245, 245, 255));
        drawText("BUILT FOR CTR-001", 143.0f, 208.0f, 0.36f,
                 C2D_Color32(231, 154, 82, 255));
    } else {
        C2D_DrawRectSolid(12.0f, 12.0f, 0.3f, 106.0f, 8.0f, C2D_Color32(30, 20, 24, 220));
        C2D_DrawRectSolid(14.0f, 14.0f, 0.4f, 102.0f * world.player.health / 100.0f, 4.0f,
                          C2D_Color32(150, 35, 42, 255));
        C2D_DrawRectSolid(12.0f, 23.0f, 0.3f, 106.0f, 6.0f, C2D_Color32(25, 24, 25, 220));
        C2D_DrawRectSolid(14.0f, 25.0f, 0.4f, 102.0f * world.player.stamina / 100.0f, 2.0f,
                          C2D_Color32(66, 145, 76, 255));
        if (world.zone == Zone::Arena && world.boss.state != BossState::Dead) {
            C2D_DrawRectSolid(72.0f, 211.0f, 0.3f, 256.0f, 10.0f, C2D_Color32(18, 14, 14, 230));
            C2D_DrawRectSolid(75.0f, 214.0f, 0.4f, 250.0f * world.boss.health / 260.0f, 4.0f,
                              C2D_Color32(126, 28, 24, 255));
            drawText("THE ASHEN WARDEN", 139.0f, 190.0f, 0.40f, C2D_Color32(230, 220, 205, 255));
        }
        if (world.dialogue_active) {
            C2D_DrawRectSolid(24.0f, 158.0f, 0.3f, 352.0f, 67.0f, C2D_Color32(7, 7, 10, 225));
            drawText("VEILED KEEPER", 38.0f, 166.0f, 0.40f, C2D_Color32(196, 167, 224, 255));
            drawText(keeper_dialogue_.data(), 38.0f, 185.0f, 0.40f,
                     C2D_Color32(240, 238, 232, 255), 325.0f);
        }
        if (world.player.state == PlayerState::Dead) {
            drawText("EMBER EXTINGUISHED", 95.0f, 92.0f, 0.75f, C2D_Color32(175, 42, 38, 255));
            drawText("Press A to restart", 135.0f, 130.0f, 0.45f, C2D_Color32(225, 220, 215, 255));
        } else if (world.player.state == PlayerState::Victory) {
            drawText("WARDEN FELLED", 124.0f, 92.0f, 0.78f, C2D_Color32(222, 188, 112, 255));
            drawText("Press A to begin anew", 118.0f, 130.0f, 0.45f, C2D_Color32(225, 220, 215, 255));
        }
        if (paused) {
            C2D_DrawRectSolid(0.0f, 0.0f, 0.6f, 400.0f, 240.0f, C2D_Color32(0, 0, 0, 155));
            drawText("PAUSED", 163.0f, 96.0f, 0.78f, C2D_Color32(245, 245, 245, 255));
        }
        if (world.arena_transition && world.transition_timer > 0.0f) {
            const float progress = 1.0f - world.transition_timer / 0.85f;
            const u8 alpha = static_cast<u8>(std::fmax(0.0f, std::fmin(220.0f, progress * 255.0f)));
            C2D_DrawRectSolid(0.0f, 0.0f, 0.8f, 400.0f, 240.0f, C2D_Color32(8, 6, 10, alpha));
        }
    }

    C2D_TargetClear(bottom_target_, C2D_Color32(17, 15, 21, 255));
    C2D_SceneBegin(bottom_target_);
    drawText(ZoneManager::name(world.zone), 16.0f, 12.0f, 0.50f, C2D_Color32(222, 188, 112, 255));
    char status[256];
    std::snprintf(status, sizeof(status),
                  "HP %.0f   ST %.0f   FLASKS %d\nQUICK: %s  (D-up/down)\nA interact  B dodge/run  X heal\nR light  Y heavy  L lock  START pause\nD-left/right camera   SELECT exit\nTap lower-right: diagnostics",
                  world.player.health, world.player.stamina, world.player.flasks,
                  quickItemName(world.player.selected_item));
    drawText(status, 16.0f, 43.0f, 0.42f, C2D_Color32(222, 222, 228, 255), 292.0f);
    for (int index = 0; index < world.player.flasks; ++index) {
        C2D_DrawRectSolid(20.0f + static_cast<float>(index) * 28.0f, 178.0f, 0.2f,
                          20.0f, 34.0f, C2D_Color32(190, 108, 28, 255));
    }
    if (world.debug_overlay) {
        char diagnostics[256];
        std::snprintf(diagnostics, sizeof(diagnostics),
                      "%s  %.1f ms  draws %u  visible %u  culled %u\nzone data %u KB  declared %lu KB  peak %u KB\nlinear free %lu KB  audio %s  underruns %u",
                      hardware_model_,
                      frame_ms, draw_calls_, visible_objects_, culled_objects_,
                      (zone_resource_bytes + 1023U) / 1024U,
                      static_cast<unsigned long>(world.zone_resident_bytes / 1024U), zone_memory_kb,
                      static_cast<unsigned long>(linearSpaceFree() / 1024U),
                      audio_available ? "streaming" : "unavailable", audio_underruns);
        C2D_DrawRectSolid(8.0f, 193.0f, 0.3f, 304.0f, 45.0f, C2D_Color32(4, 4, 6, 235));
        drawText(diagnostics, 12.0f, 196.0f, 0.32f, C2D_Color32(116, 230, 152, 255));
    }
}

void Renderer::shutdown() {
    if (text_buffer_) {
        C2D_TextBufDelete(text_buffer_);
        text_buffer_ = nullptr;
    }
    if (vbo_data_) {
        linearFree(vbo_data_);
        vbo_data_ = nullptr;
    }
    if (index_data_) {
        linearFree(index_data_);
        index_data_ = nullptr;
    }
    if (environment_atlas_) {
        Tex3DS_TextureFree(environment_atlas_);
        environment_atlas_ = nullptr;
        C3D_TexDelete(&environment_texture_);
    }
    if (vertex_shader_) {
        shaderProgramFree(&program_);
        DVLB_Free(vertex_shader_);
        vertex_shader_ = nullptr;
    }
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

} // namespace demake
