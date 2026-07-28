#include "demake/renderer.hpp"

#include <algorithm>
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

constexpr float kCameraDistance = 7.2f;
constexpr float kExteriorCameraHeight = 4.5f;
constexpr float kInteriorCameraHeight = 3.45f;
constexpr float kInteriorCameraWallLimit = 4.55f;
constexpr float kInteriorCameraFrontLimit = 4.25f;
constexpr float kCameraOccluderPadding = 0.18f;

float distanceToSegment(Vec2 point, Vec2 start, Vec2 end) {
    const Vec2 segment{end.x - start.x, end.z - start.z};
    const float length_squared = segment.x * segment.x + segment.z * segment.z;
    if (length_squared <= 0.0001f) {
        return distance(point, start);
    }
    const Vec2 offset{point.x - start.x, point.z - start.z};
    const float projection = std::clamp(
        (offset.x * segment.x + offset.z * segment.z) / length_squared, 0.0f, 1.0f);
    return distance(point, {start.x + segment.x * projection,
                            start.z + segment.z * projection});
}

const char* objectiveFor(const WorldState& world) {
    if (world.player.state == PlayerState::Dead) {
        return "Return to the ember";
    }
    if (world.zone == Zone::Arena &&
        (world.player.state == PlayerState::Victory || world.boss.state == BossState::Dead)) {
        return "The Ashen Warden is felled";
    }
    if (world.zone == Zone::Interior) {
        if (!world.door_activated) {
            return "Reach the sealed lift and press ACT";
        }
        if (world.door_progress < 0.95f) {
            return "The lift gate is opening";
        }
        return "Enter the Sable Expanse";
    }
    if (world.zone == Zone::Vista) {
        if (world.dialogue_active) {
            return "ACT: continue   B: leave";
        }
        if (!world.dialogue_complete) {
            return "Find the Veiled Keeper";
        }
        if (world.arena_transition) {
            return "Crossing the pale gate";
        }
        return "Cross the pale gate and press ACT";
    }
    if (world.zone == Zone::Field) {
        if (world.player.mounted) {
            return "East: boar ravine   West: cloud mountain";
        }
        float nearest_distance = distance(world.player.position, world.horses[0].position);
        for (unsigned index = 1; index < kFieldHorseCount; ++index) {
            nearest_distance = std::min(
                nearest_distance, distance(world.player.position, world.horses[index].position));
        }
        if (nearest_distance < 2.6f) {
            return "Press ACT to mount this horse";
        }
        return "Ride east to the ravine or west to Cloudbreak";
    }
    if (world.zone == Zone::BoarValley) {
        return world.boar_defeated ? "The valley is quiet - return south"
                                   : "Hunt Gore-Tusk in the open ravine";
    }
    if (world.zone == Zone::CloudPlateau) {
        if (world.ogre_defeated) {
            return "The summit is yours - descend south";
        }
        if (world.player.position.z < 76.0f) {
            return "Ride the ragged path up through the clouds";
        }
        return "Defeat Arashi and evade the violet runes";
    }
    return world.player.lock_on ? "Defeat the Warden - target locked"
                                : "Defeat the Warden - tap LOCK";
}

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
                  0.1f, 280.0f, false);

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
    if (player.lock_on && isBossZone(world.zone) && world.boss.state != BossState::Dead) {
        camera_yaw_ = std::atan2(world.boss.position.x - player.position.x,
                                 world.boss.position.z - player.position.z);
    } else {
        camera_yaw_ += 0.0f;
    }
    const float forward_x = std::sin(camera_yaw_);
    const float forward_z = std::cos(camera_yaw_);
    const bool wide_camera = world.zone == Zone::Field || world.zone == Zone::CloudPlateau;
    const float camera_distance = wide_camera ? 8.6f : kCameraDistance;
    camera_ground_ = {player.position.x - forward_x * camera_distance,
                      player.position.z - forward_z * camera_distance};
    float camera_height = kExteriorCameraHeight;
    if (world.zone == Zone::Interior) {
        // The vestibule roof starts at Y=3.95. Keeping the camera below it
        // prevents the roof slab from sitting between the camera and player.
        // Constraining the orbit to the room's inner side-wall and door faces
        // also prevents rotated views from looking back through those solids.
        camera_ground_.x = std::clamp(camera_ground_.x,
                                      -kInteriorCameraWallLimit,
                                      kInteriorCameraWallLimit);
        camera_ground_.z = std::min(camera_ground_.z, kInteriorCameraFrontLimit);
        camera_height = kInteriorCameraHeight;
    } else if (wide_camera) {
        camera_height = zoneGroundHeight(world.zone, player.position) +
                        (player.mounted ? 5.5f : 5.0f);
    }
    const C3D_FVec camera = FVec3_New(camera_ground_.x, camera_height, camera_ground_.z);
    const float target_y = wide_camera
                               ? zoneGroundHeight(world.zone, player.position) +
                                     (player.mounted ? 3.3f : 3.0f)
                               : 1.1f;
    const float target_forward = wide_camera ? 7.5f : 1.2f;
    const C3D_FVec target =
        FVec3_New(player.position.x + forward_x * target_forward,
                  target_y,
                  player.position.z + forward_z * target_forward);
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

    const u32 clear_color = world.zone == Zone::Interior
                                ? 0x30283BFF
                                : (world.zone == Zone::Field ||
                                           world.zone == Zone::CloudPlateau
                                       ? 0xA9D4EEFF
                                       : (world.zone == Zone::BoarValley
                                              ? 0x91B6C8FF
                                              : 0x8A786FFF));
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
    renderStaticScene(world, scene_boxes, scene_box_count);
    switch (world.zone) {
        case Zone::Interior: renderInterior(world); break;
        case Zone::Vista: renderVista(world); break;
        case Zone::Arena: renderArena(world); break;
        case Zone::Field: renderField(world); break;
        case Zone::BoarValley: renderBoarValley(world); break;
        case Zone::CloudPlateau: renderCloudPlateau(world); break;
    }
    if (world.zone == Zone::Field) {
        for (unsigned index = 0; index < kFieldHorseCount; ++index) {
            const FieldHorse& horse = world.horses[index];
            renderHorse(
                horse.position, horse.facing, world.elapsed,
                world.player.mounted && world.active_horse == index &&
                    world.player.state == PlayerState::Move,
                index);
        }
        renderWildlife(world);
    } else if (world.zone == Zone::CloudPlateau) {
        const FieldHorse& horse = world.horses[world.active_horse];
        renderHorse(horse.position, horse.facing, world.elapsed,
                    world.player.mounted && world.player.state == PlayerState::Move,
                    world.active_horse,
                    zoneGroundHeight(world.zone, horse.position));
    }
    Player displayed_player = world.player;
    if (displayed_player.mounted) {
        displayed_player.state = PlayerState::Idle;
    }
    RigidPose player_pose{};
    samplePlayerPose(displayed_player, world.elapsed, player_pose);
    renderPlayer(displayed_player, player_pose,
                 zoneGroundHeight(world.zone, displayed_player.position) +
                     (displayed_player.mounted ? 1.55f : 0.0f));
}

void Renderer::renderPanorama(Zone zone) {
    if (zone == Zone::Interior) {
        return;
    }
    const bool grand_outdoor = zone == Zone::Field || zone == Zone::CloudPlateau;
    const float red = zone == Zone::Vista ? 0.52f
                      : (grand_outdoor ? 0.53f
                         : (zone == Zone::BoarValley ? 0.43f : 0.34f));
    const float green = zone == Zone::Vista ? 0.46f
                        : (grand_outdoor ? 0.73f
                           : (zone == Zone::BoarValley ? 0.61f : 0.29f));
    const float blue = zone == Zone::Vista ? 0.58f
                       : (grand_outdoor ? 0.91f
                          : (zone == Zone::BoarValley ? 0.70f : 0.34f));
    const float back_z = grand_outdoor ? 205.0f : 78.0f;
    const float side_x = grand_outdoor ? 112.0f : 58.0f;
    const float side_z = grand_outdoor ? 80.0f : 34.0f;
    const float sky_height = grand_outdoor ? 82.0f : 38.0f;
    const float side_depth = grand_outdoor ? 270.0f : 120.0f;
    drawBox(0.0f, sky_height * 0.5f, back_z, side_x * 2.0f, sky_height, 1.0f,
            0.0f, red, green, blue, true);
    drawBox(-side_x, sky_height * 0.5f, side_z, 1.0f, sky_height, side_depth,
            0.0f, red * 0.72f, green * 0.72f, blue * 0.82f, true);
    drawBox(side_x, sky_height * 0.5f, side_z, 1.0f, sky_height, side_depth,
            0.0f, red * 0.78f, green * 0.78f, blue * 0.88f, true);
}

void Renderer::renderStaticScene(const WorldState& world, const SceneBox* boxes,
                                 std::size_t count) {
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
            // Generated columns and trees are scenery, not gameplay barriers.
            // Hide one only while its footprint crosses the camera-to-player
            // segment, preventing the same full-screen obstruction in any zone.
            const float occluder_radius =
                0.5f * std::sqrt(box.sx * box.sx + box.sz * box.sz) +
                kCameraOccluderPadding;
            if (distanceToSegment({box.x, box.z}, camera_ground_, world.player.position) <
                occluder_radius) {
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
    renderWarden(world.boss, world.elapsed);
}

void Renderer::renderField(const WorldState& world) {
    const float cloud_drift = std::sin(world.elapsed * 0.10f) * 4.0f;
    const float high_drift = std::sin(world.elapsed * 0.065f + 1.4f) * 5.0f;
    drawBox(-24.0f, 35.0f, 83.0f, 5.8f, 5.8f, 0.8f,
            0.0f, 1.00f, 0.84f, 0.30f, true);
    drawBox(-25.0f + cloud_drift, 27.0f, 69.0f, 9.5f, 2.20f, 2.4f,
            0.08f, 0.94f, 0.96f, 0.93f, true);
    drawBox(-18.0f + cloud_drift, 29.0f, 71.0f, 6.8f, 4.00f, 2.8f,
            -0.06f, 0.98f, 0.98f, 0.96f, true);
    drawBox(-11.0f + cloud_drift, 27.4f, 72.0f, 8.0f, 2.40f, 2.5f,
            0.04f, 0.95f, 0.97f, 0.96f, true);
    drawBox(19.0f - cloud_drift * 0.7f, 25.5f, 79.0f, 10.5f, 2.50f, 2.6f,
            -0.04f, 0.91f, 0.95f, 0.95f, true);
    drawBox(27.0f - cloud_drift * 0.7f, 27.8f, 81.0f, 7.2f, 4.20f, 3.0f,
            0.07f, 0.97f, 0.98f, 0.97f, true);
    drawBox(35.0f - cloud_drift * 0.7f, 25.8f, 80.0f, 8.6f, 2.30f, 2.5f,
            -0.05f, 0.93f, 0.96f, 0.96f, true);
    drawBox(-3.0f + high_drift, 35.0f, 119.0f, 13.0f, 2.40f, 3.2f,
            0.03f, 0.90f, 0.94f, 0.96f, true);
    drawBox(6.0f + high_drift, 37.0f, 121.0f, 8.0f, 4.20f, 3.5f,
            -0.04f, 0.97f, 0.98f, 0.98f, true);
    drawBox(15.0f + high_drift, 35.3f, 120.0f, 10.0f, 2.50f, 3.0f,
            0.06f, 0.92f, 0.96f, 0.97f, true);
    drawBox(-61.0f + cloud_drift * 0.45f, 24.0f, 74.0f, 12.0f, 2.50f, 3.0f,
            -0.03f, 0.92f, 0.96f, 0.97f, true);
    drawBox(-53.0f + cloud_drift * 0.45f, 25.8f, 76.0f, 7.0f, 3.80f, 3.2f,
            0.04f, 0.98f, 0.98f, 0.97f, true);
    drawBox(58.0f - cloud_drift * 0.35f, 22.5f, 78.0f, 13.0f, 2.40f, 3.1f,
            0.05f, 0.91f, 0.95f, 0.97f, true);
    drawBox(67.0f - cloud_drift * 0.35f, 24.2f, 80.0f, 7.5f, 3.60f, 3.3f,
            -0.04f, 0.97f, 0.98f, 0.98f, true);
}

void Renderer::renderBoarValley(const WorldState& world) {
    const float mist = std::sin(world.elapsed * 0.18f) * 4.0f;
    drawBox(-8.0f + mist, 7.5f, 40.0f, 18.0f, 1.1f, 3.2f,
            0.02f, 0.78f, 0.86f, 0.85f, true);
    drawBox(10.0f - mist * 0.6f, 9.2f, 48.0f, 15.0f, 1.3f, 3.5f,
            -0.03f, 0.83f, 0.88f, 0.87f, true);
    renderBoar(world.boss, world.elapsed);
}

void Renderer::renderCloudPlateau(const WorldState& world) {
    const float drift = std::sin(world.elapsed * 0.12f) * 5.0f;
    for (unsigned layer = 0; layer < 4; ++layer) {
        const float z = 8.0f + static_cast<float>(layer) * 17.0f;
        const float height = zoneGroundHeight(Zone::CloudPlateau, {0.0f, z}) + 2.0f;
        const float direction = (layer & 1U) == 0U ? 1.0f : -1.0f;
        drawBox(-11.0f + drift * direction * 0.45f, height, z,
                11.0f, 1.35f, 4.6f, 0.03f,
                0.92f, 0.96f, 0.97f, true);
        drawBox(10.5f - drift * direction * 0.35f, height + 1.0f, z + 2.0f,
                9.0f, 2.0f, 4.2f, -0.04f,
                0.98f, 0.98f, 0.97f, true);
    }
    const float summit_drift = std::sin(world.elapsed * 0.07f + 1.0f) * 6.0f;
    drawBox(-18.0f + summit_drift, 40.0f, 112.0f, 15.0f, 2.5f, 4.2f,
            0.02f, 0.93f, 0.96f, 0.98f, true);
    drawBox(12.0f - summit_drift, 43.0f, 118.0f, 19.0f, 3.0f, 4.5f,
            -0.03f, 0.97f, 0.98f, 0.99f, true);
    renderMountainOgre(world.boss, world.elapsed);
}

void Renderer::renderWildlife(const WorldState& world) {
    for (unsigned index = 0; index < 3; ++index) {
        const float speed = 0.38f + static_cast<float>(index) * 0.06f;
        const float phase = world.elapsed * speed + static_cast<float>(index) * 2.1f;
        const Vec2 center{-32.0f + static_cast<float>(index) * 31.0f,
                          22.0f + static_cast<float>(index) * 33.0f};
        const Vec2 position{center.x + std::sin(phase) * 13.0f,
                            center.z + std::cos(phase * 0.83f) * 9.0f};
        const float velocity_x = std::cos(phase) * 13.0f * speed;
        const float velocity_z = -std::sin(phase * 0.83f) * 9.0f * 0.83f * speed;
        renderDeer(position, std::atan2(velocity_x, velocity_z), world.elapsed,
                   index == 0 ? 0.82f : 1.0f);
    }
    for (unsigned index = 0; index < 2; ++index) {
        const float phase = world.elapsed * (0.58f + index * 0.09f) + index * 3.2f;
        const Vec2 center{index == 0 ? 28.0f : -46.0f, index == 0 ? 35.0f : 91.0f};
        const Vec2 position{center.x + std::sin(phase) * 9.0f,
                            center.z + std::cos(phase) * 6.0f};
        renderFox(position, phase + 1.57f, world.elapsed);
    }
    for (unsigned index = 0; index < 3; ++index) {
        const float phase = world.elapsed * 0.22f + index * 2.05f;
        const Vec2 position{-13.0f + std::sin(phase) * 8.0f,
                            48.0f + static_cast<float>(index) * 7.0f +
                                std::cos(phase) * 4.0f};
        renderCrane(position, phase, world.elapsed + index);
    }
}

void Renderer::renderPlayer(const Player& player, const RigidPose& pose, float vertical_offset) {
    constexpr float scale = 1.0f;
    const Vec2 position = player.position;
    const float facing = player.facing;
    const float side_x = std::cos(facing);
    const float side_z = -std::sin(facing);
    const float forward_x = std::sin(facing);
    const float forward_z = std::cos(facing);
    const float root_y = pose.at(Bone::Root).vertical * scale;

    // Keep the animated 15-bone body, then spend the remaining box budget on
    // the player's readable identity: slit helm, mantle, tabard, and short sword.
    renderHumanoid(position, facing, scale, pose, 0.47f, 0.49f, 0.56f, false,
                   vertical_offset);

    for (int side = -1; side <= 1; side += 2) {
        drawBox(position.x + side_x * side * 0.48f,
                1.73f + root_y + vertical_offset,
                position.z + side_z * side * 0.48f,
                0.34f, 0.22f, 0.42f, facing,
                0.31f, 0.33f, 0.38f);
    }

    const float torso_forward = pose.at(Bone::Torso).forward;
    drawBox(position.x + forward_x * (torso_forward - 0.24f),
            1.72f + root_y + vertical_offset,
            position.z + forward_z * (torso_forward - 0.24f),
            0.88f, 0.16f, 0.54f, facing,
            0.24f, 0.24f, 0.29f);
    drawBox(position.x - forward_x * 0.28f,
            1.30f + root_y + vertical_offset,
            position.z - forward_z * 0.28f,
            0.66f, 0.70f, 0.08f, facing,
            0.22f, 0.22f, 0.26f);

    drawBox(position.x + forward_x * (torso_forward + 0.24f),
            1.43f + root_y + vertical_offset,
            position.z + forward_z * (torso_forward + 0.24f),
            0.19f, 0.55f, 0.045f, facing,
            0.58f, 0.18f, 0.15f);
    drawBox(position.x + forward_x * 0.24f,
            0.82f + root_y + vertical_offset,
            position.z + forward_z * 0.24f,
            0.58f, 0.42f, 0.045f, facing,
            0.52f, 0.15f, 0.13f);

    const float head_forward = pose.at(Bone::Head).forward;
    const float head_y =
        2.06f + root_y + pose.at(Bone::Head).vertical + vertical_offset;
    const float visor_x = position.x + forward_x * (head_forward + 0.255f);
    const float visor_z = position.z + forward_z * (head_forward + 0.255f);
    drawBox(visor_x, head_y + 0.03f, visor_z,
            0.39f, 0.14f, 0.025f, facing,
            0.035f, 0.03f, 0.04f);
    for (int side = -1; side <= 1; side += 2) {
        drawBox(visor_x + side_x * side * 0.10f,
                head_y + 0.03f,
                visor_z + side_z * side * 0.10f,
                0.025f, 0.17f, 0.035f, facing,
                0.48f, 0.48f, 0.53f);
    }

    const BoneTransform& weapon_pose = pose.at(Bone::Weapon);
    const float weapon_yaw = facing + weapon_pose.yaw;
    const float weapon_forward_x = std::sin(weapon_yaw);
    const float weapon_forward_z = std::cos(weapon_yaw);
    const float hand_x = position.x + side_x * 0.70f +
                         forward_x * (0.38f + weapon_pose.forward);
    const float hand_z = position.z + side_z * 0.70f +
                         forward_z * (0.38f + weapon_pose.forward);
    const float weapon_y = 1.13f + root_y + vertical_offset;
    drawBox(hand_x, weapon_y, hand_z,
            0.09f, 0.11f, 0.30f, weapon_yaw,
            0.22f, 0.15f, 0.10f);
    drawBox(hand_x + weapon_forward_x * 0.21f,
            weapon_y,
            hand_z + weapon_forward_z * 0.21f,
            0.48f, 0.10f, 0.10f, weapon_yaw,
            0.28f, 0.28f, 0.31f);
    drawBox(hand_x + weapon_forward_x * 0.90f,
            weapon_y,
            hand_z + weapon_forward_z * 0.90f,
            0.13f, 0.09f, 1.36f, weapon_yaw,
            0.76f, 0.77f, 0.81f);
}

void Renderer::renderHumanoid(Vec2 position, float facing, float scale, const RigidPose& pose,
                              float red, float green, float blue, bool weapon,
                              float vertical_offset) {
    if (vertical_offset <= 0.01f) {
        drawBlobShadow(position, scale);
    }
    const float side_x = std::cos(facing);
    const float side_z = -std::sin(facing);
    const float forward_x = std::sin(facing);
    const float forward_z = std::cos(facing);
    const float root_y = pose.at(Bone::Root).vertical * scale;
    drawBox(position.x, 0.98f * scale + root_y + vertical_offset, position.z,
            0.72f * scale, 0.34f * scale, 0.44f * scale,
            facing + pose.at(Bone::Pelvis).yaw, red * 0.78f, green * 0.78f, blue * 0.82f);
    drawBox(position.x + forward_x * pose.at(Bone::Torso).forward * scale,
            1.48f * scale + root_y + vertical_offset,
            position.z + forward_z * pose.at(Bone::Torso).forward * scale,
            0.75f * scale, 0.78f * scale, 0.45f * scale,
            facing + pose.at(Bone::Torso).yaw, red, green, blue);
    drawBox(position.x + forward_x * pose.at(Bone::Head).forward * scale,
            2.05f * scale + root_y + pose.at(Bone::Head).vertical * scale + vertical_offset,
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
                0.65f * scale + root_y + vertical_offset,
                position.z + side_z * side * 0.22f * scale + forward_z * leg_stride,
                0.24f * scale, 0.48f * scale, 0.27f * scale,
                facing + pose.at(upper_leg).yaw, red * 0.72f, green * 0.72f, blue * 0.77f);
        drawBox(position.x + side_x * side * 0.22f * scale + forward_x * pose.at(lower_leg).forward,
                0.25f * scale + root_y + vertical_offset,
                position.z + side_z * side * 0.22f * scale + forward_z * pose.at(lower_leg).forward,
                0.21f * scale, 0.40f * scale, 0.23f * scale,
                facing + pose.at(lower_leg).yaw, red * 0.66f, green * 0.66f, blue * 0.72f);
        drawBox(position.x + side_x * side * 0.22f * scale +
                    forward_x * (0.10f + pose.at(foot).forward) * scale,
                0.05f * scale + root_y + vertical_offset,
                position.z + side_z * side * 0.22f * scale +
                    forward_z * (0.10f + pose.at(foot).forward) * scale,
                0.24f * scale, 0.12f * scale, 0.42f * scale,
                facing + pose.at(foot).yaw, red * 0.58f, green * 0.58f, blue * 0.64f);

        const float arm_reach = pose.at(upper_arm).forward;
        drawBox(position.x + side_x * side * 0.53f * scale + forward_x * arm_reach,
                1.52f * scale + root_y + vertical_offset,
                position.z + side_z * side * 0.53f * scale + forward_z * arm_reach,
                0.23f * scale, 0.44f * scale, 0.24f * scale,
                facing + pose.at(upper_arm).yaw, red * 0.76f, green * 0.76f, blue * 0.81f);
        drawBox(position.x + side_x * side * 0.58f * scale +
                    forward_x * pose.at(lower_arm).forward,
                1.16f * scale + root_y + vertical_offset,
                position.z + side_z * side * 0.58f * scale +
                    forward_z * pose.at(lower_arm).forward,
                0.20f * scale, 0.40f * scale, 0.21f * scale,
                facing + pose.at(lower_arm).yaw, red * 0.68f, green * 0.68f, blue * 0.75f);
        drawBox(position.x + side_x * side * 0.60f * scale + forward_x * arm_reach * 1.2f,
                0.91f * scale + root_y + vertical_offset,
                position.z + side_z * side * 0.60f * scale + forward_z * arm_reach * 1.2f,
                0.18f * scale, 0.20f * scale, 0.18f * scale,
                facing, red * 0.82f, green * 0.72f, blue * 0.68f);
    }
    if (weapon) {
        const BoneTransform& weapon_pose = pose.at(Bone::Weapon);
        drawBox(position.x + side_x * 0.72f * scale +
                    forward_x * (0.8f + weapon_pose.forward) * scale,
                1.25f * scale + root_y + vertical_offset,
                position.z + side_z * 0.72f * scale +
                    forward_z * (0.8f + weapon_pose.forward) * scale,
                0.10f * scale, 0.12f * scale, 1.85f * scale,
                facing + weapon_pose.yaw, 0.72f, 0.74f, 0.78f);
    }
}

void Renderer::renderHorse(Vec2 position, float facing, float elapsed, bool moving,
                           unsigned coat, float vertical_offset) {
    if (distance(camera_ground_, position) > 54.0f) {
        return;
    }
    drawBlobShadow(position, 1.55f, vertical_offset);
    const float side_x = std::cos(facing);
    const float side_z = -std::sin(facing);
    const float forward_x = std::sin(facing);
    const float forward_z = std::cos(facing);
    const float gait = moving ? elapsed * 11.0f : elapsed * 1.5f;
    const float body_bob = moving ? std::sin(gait * 2.0f) * 0.045f : 0.0f;
    constexpr float coats[kFieldHorseCount][3] = {
        {0.52f, 0.27f, 0.13f},
        {0.23f, 0.22f, 0.21f},
        {0.72f, 0.71f, 0.64f},
    };
    const unsigned palette = coat % kFieldHorseCount;
    const float coat_red = coats[palette][0];
    const float coat_green = coats[palette][1];
    const float coat_blue = coats[palette][2];

    drawBox(position.x, 1.02f + body_bob + vertical_offset, position.z,
            0.92f, 0.86f, 1.82f, facing,
            coat_red, coat_green, coat_blue);
    drawBox(position.x, 1.48f + body_bob + vertical_offset, position.z - forward_z * 0.10f,
            0.78f, 0.18f, 1.20f, facing,
            0.22f, 0.13f, 0.08f);
    drawBox(position.x + forward_x * 0.88f, 1.47f + body_bob + vertical_offset,
            position.z + forward_z * 0.88f,
            0.56f, 1.02f, 0.64f, facing,
            coat_red * 1.06f, coat_green * 1.06f, coat_blue * 1.06f);
    drawBox(position.x + forward_x * 1.30f, 1.86f + body_bob + vertical_offset,
            position.z + forward_z * 1.30f,
            0.64f, 0.56f, 0.86f, facing,
            coat_red * 1.10f, coat_green * 1.10f, coat_blue * 1.10f);
    drawBox(position.x + forward_x * 1.73f, 1.72f + body_bob + vertical_offset,
            position.z + forward_z * 1.73f,
            0.50f, 0.34f, 0.52f, facing,
            coat_red * 1.16f, coat_green * 1.16f, coat_blue * 1.16f);
    for (int side = -1; side <= 1; side += 2) {
        drawBox(position.x + forward_x * 1.22f + side_x * side * 0.22f,
                2.22f + body_bob + vertical_offset,
                position.z + forward_z * 1.22f + side_z * side * 0.22f,
                0.16f, 0.42f, 0.16f, facing + side * 0.08f,
                coat_red * 0.62f, coat_green * 0.65f, coat_blue * 0.68f);
    }
    drawBox(position.x - forward_x * 1.18f, 1.12f + body_bob + vertical_offset,
            position.z - forward_z * 1.18f,
            0.22f, 0.22f, 1.18f, facing,
            coat_red * 0.48f, coat_green * 0.52f, coat_blue * 0.55f);

    for (int front = -1; front <= 1; front += 2) {
        for (int side = -1; side <= 1; side += 2) {
            const float phase = gait + static_cast<float>((front + side) * 2);
            const float lift = moving ? std::fmax(0.0f, std::sin(phase)) * 0.18f : 0.0f;
            const float along = static_cast<float>(front) * 0.62f;
            const float leg_x =
                position.x + forward_x * along + side_x * static_cast<float>(side) * 0.33f;
            const float leg_z =
                position.z + forward_z * along + side_z * static_cast<float>(side) * 0.33f;
            drawBox(leg_x, 0.48f + lift + vertical_offset, leg_z,
                    0.19f, 0.76f, 0.21f, facing,
                    coat_red * 0.82f, coat_green * 0.84f, coat_blue * 0.86f);
            drawBox(leg_x + forward_x * 0.08f, 0.10f + lift + vertical_offset,
                    leg_z + forward_z * 0.08f,
                    0.24f, 0.16f, 0.38f, facing,
                    0.18f, 0.14f, 0.11f);
        }
    }
}

void Renderer::renderDeer(Vec2 position, float facing, float elapsed, float scale) {
    if (distance(camera_ground_, position) > 48.0f) {
        return;
    }
    drawBlobShadow(position, scale);
    const float forward_x = std::sin(facing);
    const float forward_z = std::cos(facing);
    const float side_x = std::cos(facing);
    const float side_z = -std::sin(facing);
    const float stride = std::sin(elapsed * 8.0f + position.x) * 0.12f;
    drawBox(position.x, 0.86f * scale, position.z,
            0.72f * scale, 0.72f * scale, 1.38f * scale, facing,
            0.58f, 0.38f, 0.20f);
    drawBox(position.x + forward_x * 0.70f * scale, 1.24f * scale,
            position.z + forward_z * 0.70f * scale,
            0.42f * scale, 0.82f * scale, 0.42f * scale, facing,
            0.64f, 0.43f, 0.24f);
    drawBox(position.x + forward_x * 1.02f * scale, 1.58f * scale,
            position.z + forward_z * 1.02f * scale,
            0.48f * scale, 0.42f * scale, 0.62f * scale, facing,
            0.67f, 0.46f, 0.27f);
    for (int front = -1; front <= 1; front += 2) {
        for (int side = -1; side <= 1; side += 2) {
            const float along = static_cast<float>(front) * 0.43f * scale;
            const float swing = stride * static_cast<float>(front * side);
            drawBox(position.x + forward_x * (along + swing) +
                        side_x * static_cast<float>(side) * 0.24f * scale,
                    0.34f * scale,
                    position.z + forward_z * (along + swing) +
                        side_z * static_cast<float>(side) * 0.24f * scale,
                    0.12f * scale, 0.68f * scale, 0.14f * scale, facing,
                    0.42f, 0.29f, 0.18f);
        }
    }
    for (int side = -1; side <= 1; side += 2) {
        drawBox(position.x + forward_x * 1.04f * scale +
                    side_x * static_cast<float>(side) * 0.18f * scale,
                1.96f * scale,
                position.z + forward_z * 1.04f * scale +
                    side_z * static_cast<float>(side) * 0.18f * scale,
                0.08f * scale, 0.56f * scale, 0.08f * scale,
                facing + static_cast<float>(side) * 0.18f,
                0.36f, 0.25f, 0.15f);
    }
}

void Renderer::renderFox(Vec2 position, float facing, float elapsed) {
    if (distance(camera_ground_, position) > 42.0f) {
        return;
    }
    drawBlobShadow(position, 0.65f);
    const float forward_x = std::sin(facing);
    const float forward_z = std::cos(facing);
    const float tail_sway = std::sin(elapsed * 5.0f + position.z) * 0.22f;
    drawBox(position.x, 0.38f, position.z,
            0.42f, 0.48f, 0.92f, facing, 0.78f, 0.31f, 0.10f);
    drawBox(position.x + forward_x * 0.58f, 0.53f,
            position.z + forward_z * 0.58f,
            0.38f, 0.38f, 0.48f, facing, 0.86f, 0.38f, 0.12f);
    drawBox(position.x + forward_x * 0.82f, 0.48f,
            position.z + forward_z * 0.82f,
            0.26f, 0.22f, 0.34f, facing, 0.92f, 0.50f, 0.22f);
    drawBox(position.x - forward_x * 0.68f, 0.52f,
            position.z - forward_z * 0.68f,
            0.30f, 0.30f, 0.92f, facing + tail_sway,
            0.72f, 0.27f, 0.09f);
    drawBox(position.x - forward_x * 1.03f, 0.52f,
            position.z - forward_z * 1.03f,
            0.28f, 0.28f, 0.36f, facing + tail_sway,
            0.86f, 0.75f, 0.55f);
}

void Renderer::renderCrane(Vec2 position, float facing, float elapsed) {
    if (distance(camera_ground_, position) > 45.0f) {
        return;
    }
    const float sway = std::sin(elapsed * 2.0f) * 0.08f;
    const float forward_x = std::sin(facing);
    const float forward_z = std::cos(facing);
    drawBox(position.x, 0.78f, position.z,
            0.42f, 0.66f, 0.62f, facing, 0.88f, 0.90f, 0.86f);
    drawBox(position.x + forward_x * 0.26f, 1.32f,
            position.z + forward_z * 0.26f,
            0.17f, 0.72f, 0.18f, facing + sway, 0.91f, 0.92f, 0.89f);
    drawBox(position.x + forward_x * 0.42f, 1.72f,
            position.z + forward_z * 0.42f,
            0.30f, 0.26f, 0.32f, facing, 0.16f, 0.18f, 0.17f);
    drawBox(position.x, 0.25f, position.z,
            0.08f, 0.84f, 0.08f, facing, 0.42f, 0.30f, 0.20f);
}

void Renderer::drawBlobShadow(Vec2 position, float scale, float vertical_offset) {
    drawBox(position.x, 0.012f + vertical_offset, position.z,
            1.15f * scale, 0.025f, 0.72f * scale,
            0.0f, 0.055f, 0.045f, 0.055f, true);
}

void Renderer::renderBoar(const Boss& boss, float elapsed) {
    const float forward_x = std::sin(boss.facing);
    const float forward_z = std::cos(boss.facing);
    const float side_x = std::cos(boss.facing);
    const float side_z = -std::sin(boss.facing);
    if (boss.state == BossState::Dead) {
        drawBlobShadow(boss.position, 1.8f);
        drawBox(boss.position.x, 0.62f, boss.position.z,
                2.6f, 0.85f, 3.4f, boss.facing + 1.15f,
                0.25f, 0.16f, 0.10f, true);
        return;
    }
    const float charge_bob =
        boss.state == BossState::Slash ? std::sin(elapsed * 18.0f) * 0.12f : 0.0f;
    drawBlobShadow(boss.position, 2.0f);
    drawBox(boss.position.x, 1.20f + charge_bob, boss.position.z,
            2.45f, 1.75f, 3.55f, boss.facing,
            0.34f, 0.20f, 0.12f);
    drawBox(boss.position.x - forward_x * 0.75f, 1.95f + charge_bob,
            boss.position.z - forward_z * 0.75f,
            2.25f, 0.85f, 1.9f, boss.facing,
            0.20f, 0.14f, 0.10f);
    drawBox(boss.position.x + forward_x * 1.62f, 1.36f + charge_bob,
            boss.position.z + forward_z * 1.62f,
            1.90f, 1.65f, 1.65f, boss.facing,
            0.39f, 0.23f, 0.13f);
    drawBox(boss.position.x + forward_x * 2.36f, 1.08f + charge_bob,
            boss.position.z + forward_z * 2.36f,
            1.25f, 0.82f, 1.08f, boss.facing,
            0.43f, 0.25f, 0.14f);
    for (int side = -1; side <= 1; side += 2) {
        drawBox(boss.position.x + forward_x * 2.42f + side_x * side * 0.72f,
                0.98f + charge_bob,
                boss.position.z + forward_z * 2.42f + side_z * side * 0.72f,
                0.24f, 0.24f, 1.35f,
                boss.facing + side * 0.32f,
                0.88f, 0.82f, 0.63f);
        drawBox(boss.position.x + forward_x * 1.40f + side_x * side * 0.70f,
                2.42f + charge_bob,
                boss.position.z + forward_z * 1.40f + side_z * side * 0.70f,
                0.34f, 0.72f, 0.30f, boss.facing + side * 0.12f,
                0.24f, 0.16f, 0.11f);
    }
    for (int front = -1; front <= 1; front += 2) {
        for (int side = -1; side <= 1; side += 2) {
            const float along = static_cast<float>(front) * 1.05f;
            drawBox(boss.position.x + forward_x * along + side_x * side * 0.78f,
                    0.42f,
                    boss.position.z + forward_z * along + side_z * side * 0.78f,
                    0.44f, 0.84f, 0.50f, boss.facing,
                    0.27f, 0.17f, 0.11f);
        }
    }
}

void Renderer::renderMountainOgre(const Boss& boss, float elapsed) {
    const float ground = zoneGroundHeight(Zone::CloudPlateau, boss.position);
    const float forward_x = std::sin(boss.facing);
    const float forward_z = std::cos(boss.facing);
    const float side_x = std::cos(boss.facing);
    const float side_z = -std::sin(boss.facing);
    if (boss.state == BossState::Dead) {
        drawBlobShadow(boss.position, 2.4f, ground);
        drawBox(boss.position.x, ground + 0.72f, boss.position.z,
                3.8f, 1.2f, 3.0f, boss.facing + 1.18f,
                0.30f, 0.34f, 0.32f, true);
        return;
    }

    drawBlobShadow(boss.position, 2.3f, ground);
    RigidPose pose{};
    sampleBossPose(boss, elapsed, pose);
    renderHumanoid(boss.position, boss.facing, 2.45f, pose,
                   0.40f, 0.46f, 0.41f, false, ground);
    drawBox(boss.position.x, ground + 4.48f, boss.position.z,
            2.0f, 1.32f, 1.65f, boss.facing,
            0.31f, 0.37f, 0.33f);
    for (int side = -1; side <= 1; side += 2) {
        drawBox(boss.position.x + side_x * side * 1.05f - forward_x * 0.10f,
                ground + 5.48f,
                boss.position.z + side_z * side * 1.05f - forward_z * 0.10f,
                0.34f, 1.25f, 0.34f,
                boss.facing + side * 0.38f,
                0.73f, 0.68f, 0.55f);
        drawBox(boss.position.x + side_x * side * 1.32f,
                ground + 3.45f,
                boss.position.z + side_z * side * 1.32f,
                1.05f, 0.75f, 1.10f, boss.facing,
                0.25f, 0.28f, 0.27f);
    }
    const float club_yaw =
        boss.facing + (boss.state == BossState::WindupSlam ? -0.85f : 0.25f);
    const float club_x = boss.position.x + side_x * 1.82f + forward_x * 0.35f;
    const float club_z = boss.position.z + side_z * 1.82f + forward_z * 0.35f;
    drawBox(club_x, ground + 3.05f, club_z,
            0.52f, 0.58f, 4.8f, club_yaw,
            0.25f, 0.18f, 0.12f);
    drawBox(club_x + std::sin(club_yaw) * 2.1f, ground + 3.05f,
            club_z + std::cos(club_yaw) * 2.1f,
            1.35f, 1.15f, 1.55f, club_yaw,
            0.31f, 0.29f, 0.27f);

    if (boss.state == BossState::WindupMagic || boss.state == BossState::Magic) {
        const float pulse = 0.65f + std::sin(elapsed * 9.0f) * 0.25f;
        for (unsigned rune = 0; rune < 6; ++rune) {
            const float angle = elapsed * 1.8f + static_cast<float>(rune) * 1.0472f;
            drawBox(boss.position.x + std::sin(angle) * 3.2f,
                    ground + 2.2f + std::sin(angle * 2.0f) * 0.7f,
                    boss.position.z + std::cos(angle) * 3.2f,
                    0.48f, 0.48f, 0.48f, angle,
                    0.55f + pulse * 0.25f, 0.24f, 0.88f);
        }
        const float target_ground =
            zoneGroundHeight(Zone::CloudPlateau, boss.magic_target);
        drawBox(boss.magic_target.x, target_ground + 0.05f, boss.magic_target.z,
                5.2f, 0.10f, 0.30f, elapsed * 1.4f,
                0.67f, 0.28f, 0.94f, true);
        drawBox(boss.magic_target.x, target_ground + 0.06f, boss.magic_target.z,
                0.30f, 0.11f, 5.2f, -elapsed * 1.1f,
                0.34f, 0.65f, 0.96f, true);
        if (boss.state == BossState::Magic) {
            drawBox(boss.magic_target.x, target_ground + 5.0f, boss.magic_target.z,
                    1.0f, 10.0f, 1.0f, 0.0f,
                    0.64f, 0.31f, 0.92f, true);
        }
    }
}

void Renderer::renderWarden(const Boss& boss, float elapsed) {
    if (boss.state == BossState::Dead) {
        const float side_x = std::cos(boss.facing);
        const float side_z = -std::sin(boss.facing);
        const float forward_x = std::sin(boss.facing);
        const float forward_z = std::cos(boss.facing);
        drawBox(boss.position.x, 0.34f, boss.position.z,
                2.3f, 0.52f, 1.25f, boss.facing + 1.30f,
                0.16f, 0.16f, 0.18f, true);
        drawBox(boss.position.x + side_x * 1.15f,
                0.28f,
                boss.position.z + side_z * 1.15f,
                0.72f, 0.55f, 0.72f, boss.facing + 0.40f,
                0.18f, 0.18f, 0.20f, true);
        drawBox(boss.position.x - side_x * 0.85f + forward_x * 0.25f,
                0.24f,
                boss.position.z - side_z * 0.85f + forward_z * 0.25f,
                0.78f, 0.40f, 0.82f, boss.facing - 0.55f,
                0.13f, 0.13f, 0.15f, true);
        drawBox(boss.position.x + forward_x * 0.80f,
                0.36f,
                boss.position.z + forward_z * 0.80f,
                0.80f, 0.70f, 0.72f, boss.facing + 0.95f,
                0.19f, 0.18f, 0.20f, true);
        drawBox(boss.position.x - side_x * 1.25f - forward_x * 0.90f,
                0.13f,
                boss.position.z - side_z * 1.25f - forward_z * 0.90f,
                0.42f, 0.15f, 3.30f, boss.facing - 0.72f,
                0.31f, 0.30f, 0.32f, true);
        return;
    }
    RigidPose boss_pose{};
    sampleBossPose(boss, elapsed, boss_pose);
    renderHumanoid(boss.position, boss.facing, 1.75f,
                   boss_pose,
                   0.36f, 0.36f, 0.41f, false);
    const float side_x = std::cos(boss.facing);
    const float side_z = -std::sin(boss.facing);
    const float forward_x = std::sin(boss.facing);
    const float forward_z = std::cos(boss.facing);
    const float root_y = boss_pose.at(Bone::Root).vertical * 1.75f;

    // The Warden remains pure box geometry, but the oversized shoulders,
    // ember visor, hanging plates, and gate-blade make it readable at 400x240.
    for (int side = -1; side <= 1; side += 2) {
        drawBox(boss.position.x + side_x * side * 0.92f,
                3.08f + root_y,
                boss.position.z + side_z * side * 0.92f,
                0.92f, 0.58f, 0.94f, boss.facing,
                0.29f, 0.29f, 0.34f);
    }

    const float head_forward = boss_pose.at(Bone::Head).forward * 1.75f;
    const float head_y = 3.60f + root_y + boss_pose.at(Bone::Head).vertical * 1.75f;
    const float visor_x = boss.position.x + forward_x * (head_forward + 0.45f);
    const float visor_z = boss.position.z + forward_z * (head_forward + 0.45f);
    drawBox(visor_x, head_y + 0.05f, visor_z,
            0.62f, 0.16f, 0.035f, boss.facing,
            0.86f, 0.25f, 0.08f);
    drawBox(visor_x, head_y - 0.15f, visor_z,
            0.09f, 0.54f, 0.04f, boss.facing,
            0.72f, 0.16f, 0.06f);

    const float torso_forward = boss_pose.at(Bone::Torso).forward * 1.75f;
    drawBox(boss.position.x + forward_x * (torso_forward + 0.42f),
            2.56f + root_y,
            boss.position.z + forward_z * (torso_forward + 0.42f),
            1.06f, 0.88f, 0.10f, boss.facing,
            0.28f, 0.28f, 0.33f);
    drawBox(boss.position.x + forward_x * 0.44f,
            1.46f + root_y,
            boss.position.z + forward_z * 0.44f,
            0.48f, 1.15f, 0.10f, boss.facing,
            0.41f, 0.37f, 0.32f);
    for (int side = -1; side <= 1; side += 2) {
        drawBox(boss.position.x + side_x * side * 0.48f + forward_x * 0.28f,
                1.42f + root_y - (side > 0 ? 0.10f : 0.0f),
                boss.position.z + side_z * side * 0.48f + forward_z * 0.28f,
                0.34f, 0.94f, 0.14f, boss.facing + side * 0.05f,
                0.28f, 0.27f, 0.29f);
        drawBox(boss.position.x + side_x * side * 0.36f - forward_x * 0.50f,
                2.12f + root_y - (side > 0 ? 0.18f : 0.0f),
                boss.position.z + side_z * side * 0.36f - forward_z * 0.50f,
                0.54f, 1.56f, 0.12f, boss.facing,
                0.22f, 0.22f, 0.26f);
    }

    const BoneTransform& weapon_pose = boss_pose.at(Bone::Weapon);
    const float weapon_yaw = boss.facing + weapon_pose.yaw;
    const float weapon_forward_x = std::sin(weapon_yaw);
    const float weapon_forward_z = std::cos(weapon_yaw);
    const float hand_x = boss.position.x + side_x * 1.16f +
                         forward_x * (0.66f + weapon_pose.forward * 1.75f);
    const float hand_z = boss.position.z + side_z * 1.16f +
                         forward_z * (0.66f + weapon_pose.forward * 1.75f);
    const float weapon_y = 2.02f + root_y;
    drawBox(hand_x, weapon_y, hand_z,
            0.17f, 0.18f, 0.52f, weapon_yaw,
            0.20f, 0.13f, 0.09f);
    drawBox(hand_x + weapon_forward_x * 0.34f,
            weapon_y,
            hand_z + weapon_forward_z * 0.34f,
            1.05f, 0.20f, 0.18f, weapon_yaw,
            0.24f, 0.23f, 0.25f);
    drawBox(hand_x + weapon_forward_x * 1.76f,
            weapon_y,
            hand_z + weapon_forward_z * 1.76f,
            0.58f, 0.20f, 2.85f, weapon_yaw,
            0.48f, 0.47f, 0.51f);
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
    const float directional = std::fmin(
        1.0f, 0.90f + 0.16f * std::fmax(0.0f, std::cos(rotation_y - 0.65f)));
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
        drawText("Press A or tap ACT", 126.0f, 168.0f, 0.52f, C2D_Color32(245, 245, 245, 255));
        drawText("BUILT FOR CTR-001", 143.0f, 208.0f, 0.36f,
                 C2D_Color32(231, 154, 82, 255));
    } else {
        C2D_DrawRectSolid(12.0f, 12.0f, 0.3f, 106.0f, 8.0f, C2D_Color32(30, 20, 24, 220));
        C2D_DrawRectSolid(14.0f, 14.0f, 0.4f, 102.0f * world.player.health / 100.0f, 4.0f,
                          C2D_Color32(150, 35, 42, 255));
        C2D_DrawRectSolid(12.0f, 23.0f, 0.3f, 106.0f, 6.0f, C2D_Color32(25, 24, 25, 220));
        C2D_DrawRectSolid(14.0f, 25.0f, 0.4f, 102.0f * world.player.stamina / 100.0f, 2.0f,
                          C2D_Color32(66, 145, 76, 255));
        if (isBossZone(world.zone) && world.boss.state != BossState::Dead &&
            world.boss.state != BossState::Dormant) {
            C2D_DrawRectSolid(72.0f, 211.0f, 0.3f, 256.0f, 10.0f, C2D_Color32(18, 14, 14, 230));
            C2D_DrawRectSolid(75.0f, 214.0f, 0.4f,
                              250.0f * world.boss.health / bossMaximumHealth(world.zone), 4.0f,
                              C2D_Color32(126, 28, 24, 255));
            drawText(bossDisplayName(world.zone),
                     world.zone == Zone::CloudPlateau ? 104.0f : 118.0f,
                     190.0f, 0.40f, C2D_Color32(230, 220, 205, 255));
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
            drawText("Press A to enter the Sunlit Reach", 77.0f, 130.0f, 0.45f,
                     C2D_Color32(225, 220, 215, 255));
        }
        if (paused) {
            C2D_DrawRectSolid(0.0f, 0.0f, 0.6f, 400.0f, 240.0f, C2D_Color32(0, 0, 0, 155));
            drawText("PAUSED", 163.0f, 96.0f, 0.78f, C2D_Color32(245, 245, 245, 255));
        }
        if ((world.arena_transition || world.field_transition) &&
            world.transition_timer > 0.0f) {
            const float duration = world.field_transition ? 1.05f : 0.85f;
            const float progress = 1.0f - world.transition_timer / duration;
            const u8 alpha = static_cast<u8>(std::fmax(0.0f, std::fmin(220.0f, progress * 255.0f)));
            const u32 fade = world.field_transition ? C2D_Color32(244, 221, 154, alpha)
                                                    : C2D_Color32(8, 6, 10, alpha);
            C2D_DrawRectSolid(0.0f, 0.0f, 0.8f, 400.0f, 240.0f, fade);
        }
    }

    C2D_TargetClear(bottom_target_, C2D_Color32(17, 15, 21, 255));
    C2D_SceneBegin(bottom_target_);
    drawText(ZoneManager::name(world.zone), 16.0f, 12.0f, 0.50f, C2D_Color32(222, 188, 112, 255));
    char status[96];
    std::snprintf(status, sizeof(status), "HP %.0f  ST %.0f  FLASKS %d  %s%s",
                  world.player.health, world.player.stamina, world.player.flasks,
                  quickItemName(world.player.selected_item),
                  world.player.mounted ? "  RIDING" : "");
    drawText(status, 16.0f, 31.0f, 0.34f, C2D_Color32(215, 215, 220, 255));
    drawText(objectiveFor(world), 16.0f, 45.0f, 0.35f, C2D_Color32(196, 167, 224, 255), 196.0f);

    constexpr float map_x = 14.0f;
    constexpr float map_y = 68.0f;
    constexpr float map_width = 194.0f;
    constexpr float map_height = 126.0f;
    C2D_DrawRectSolid(map_x, map_y, 0.1f, map_width, map_height,
                      C2D_Color32(6, 7, 10, 255));
    const u32 map_color =
        world.zone == Zone::Field ? C2D_Color32(40, 74, 38, 255)
        : (world.zone == Zone::BoarValley ? C2D_Color32(48, 69, 43, 255)
        : (world.zone == Zone::CloudPlateau ? C2D_Color32(62, 79, 78, 255)
                                             : C2D_Color32(35, 31, 43, 255)));
    C2D_DrawRectSolid(map_x + 3.0f, map_y + 3.0f, 0.2f,
                      map_width - 6.0f, map_height - 6.0f, map_color);

    float min_x = -5.0f;
    float max_x = 5.0f;
    float min_z = -8.0f;
    float max_z = 6.0f;
    if (world.zone == Zone::Vista) {
        min_x = -11.0f;
        max_x = 11.0f;
        min_z = 5.8f;
        max_z = 28.2f;
    } else if (world.zone == Zone::Arena) {
        min_x = -9.5f;
        max_x = 9.5f;
        min_z = -9.5f;
        max_z = 9.5f;
    } else if (world.zone == Zone::Field) {
        min_x = -72.0f;
        max_x = 72.0f;
        min_z = -42.0f;
        max_z = 132.0f;
    } else if (world.zone == Zone::BoarValley) {
        min_x = -26.0f;
        max_x = 26.0f;
        min_z = -43.0f;
        max_z = 58.0f;
    } else if (world.zone == Zone::CloudPlateau) {
        min_x = -35.0f;
        max_x = 35.0f;
        min_z = -47.0f;
        max_z = 128.0f;
    }
    const auto mapPoint = [&](Vec2 point) {
        const float normalized_x = std::clamp((point.x - min_x) / (max_x - min_x), 0.0f, 1.0f);
        const float normalized_z = std::clamp((point.z - min_z) / (max_z - min_z), 0.0f, 1.0f);
        return Vec2{map_x + 7.0f + normalized_x * (map_width - 14.0f),
                    map_y + map_height - 7.0f - normalized_z * (map_height - 14.0f)};
    };

    if (world.zone == Zone::Field) {
        for (unsigned segment = 0; segment < 12; ++segment) {
            const float world_z = -35.0f + static_cast<float>(segment) * 14.0f;
            const float world_x =
                -15.0f + std::sin(static_cast<float>(segment) * 0.72f) * 12.0f;
            const Vec2 river_map = mapPoint({world_x, world_z});
            C2D_DrawRectSolid(river_map.x - 3.0f, river_map.z - 5.0f,
                              0.32f, 6.0f, 10.0f,
                              C2D_Color32(54, 139, 184, 255));
        }
        for (unsigned segment = 0; segment < 5; ++segment) {
            const float world_z = 42.0f + static_cast<float>(segment) * 13.0f;
            const float world_x =
                27.0f + std::sin(static_cast<float>(segment) * 0.86f) * 8.0f;
            const Vec2 stream_map = mapPoint({world_x, world_z});
            C2D_DrawRectSolid(stream_map.x - 2.0f, stream_map.z - 5.0f,
                              0.33f, 4.0f, 10.0f,
                              C2D_Color32(78, 165, 205, 255));
        }
        const Vec2 east_exit = mapPoint({70.0f, 18.0f});
        const Vec2 west_exit = mapPoint({-70.0f, 18.0f});
        C2D_DrawRectSolid(east_exit.x - 5.0f, east_exit.z - 5.0f,
                          0.35f, 10.0f, 10.0f, C2D_Color32(174, 72, 42, 255));
        C2D_DrawRectSolid(west_exit.x - 5.0f, west_exit.z - 5.0f,
                          0.35f, 10.0f, 10.0f, C2D_Color32(134, 183, 218, 255));
    } else if (world.zone == Zone::CloudPlateau) {
        for (unsigned step = 0; step < 8; ++step) {
            const float world_z = -20.0f + static_cast<float>(step) * 12.0f;
            const float world_x = (step & 1U) == 0U ? -2.8f : 3.2f;
            const Vec2 path_map = mapPoint({world_x, world_z});
            C2D_DrawRectSolid(path_map.x - 5.0f, path_map.z - 3.0f,
                              0.32f, 10.0f, 6.0f,
                              C2D_Color32(146, 125, 91, 255));
        }
    }

    Vec2 objective{0.0f, 4.6f};
    if (world.zone == Zone::Vista) {
        objective = world.dialogue_complete ? Vec2{0.0f, 28.0f} : Vec2{0.0f, 15.5f};
    } else if (world.zone == Zone::Arena) {
        objective = world.boss.position;
    } else if (world.zone == Zone::Field) {
        if (world.player.mounted) {
            objective = {0.0f, 116.0f};
        } else {
            unsigned nearest = 0;
            float nearest_distance =
                distance(world.player.position, world.horses[0].position);
            for (unsigned index = 1; index < kFieldHorseCount; ++index) {
                const float candidate =
                    distance(world.player.position, world.horses[index].position);
                if (candidate < nearest_distance) {
                    nearest = index;
                    nearest_distance = candidate;
                }
            }
            objective = world.horses[nearest].position;
        }
    } else if (world.zone == Zone::BoarValley) {
        objective = world.boar_defeated ? Vec2{0.0f, -42.0f} : world.boss.position;
    } else if (world.zone == Zone::CloudPlateau) {
        objective = world.ogre_defeated ? Vec2{0.0f, -46.0f}
                                       : (world.player.position.z < 76.0f
                                              ? Vec2{0.0f, 82.0f}
                                              : world.boss.position);
    }
    const Vec2 objective_map = mapPoint(objective);
    C2D_DrawRectSolid(objective_map.x - 4.0f, objective_map.z - 4.0f, 0.4f, 8.0f, 8.0f,
                      isBossZone(world.zone) && world.boss.state != BossState::Dead
                          ? C2D_Color32(174, 48, 42, 255)
                          : C2D_Color32(224, 178, 62, 255));
    if (world.zone == Zone::Field) {
        for (unsigned index = 0; index < kFieldHorseCount; ++index) {
            const Vec2 horse_map = mapPoint(world.horses[index].position);
            C2D_DrawRectSolid(horse_map.x - 3.0f, horse_map.z - 3.0f,
                              0.45f, 6.0f, 6.0f,
                              index == world.active_horse
                                  ? C2D_Color32(190, 109, 45, 255)
                                  : C2D_Color32(126, 82, 52, 255));
        }
    } else if (world.zone == Zone::CloudPlateau) {
        const Vec2 horse_map = mapPoint(world.horses[world.active_horse].position);
        C2D_DrawRectSolid(horse_map.x - 3.0f, horse_map.z - 3.0f,
                          0.45f, 6.0f, 6.0f,
                          C2D_Color32(190, 109, 45, 255));
    }
    const Vec2 player_map = mapPoint(world.player.position);
    C2D_DrawRectSolid(player_map.x - 3.0f, player_map.z - 3.0f, 0.5f, 7.0f, 7.0f,
                      C2D_Color32(230, 225, 210, 255));
    const Vec2 facing_map = mapPoint({world.player.position.x + std::sin(world.player.facing) * 0.7f,
                                     world.player.position.z + std::cos(world.player.facing) * 0.7f});
    C2D_DrawRectSolid(facing_map.x - 1.5f, facing_map.z - 1.5f, 0.5f, 3.0f, 3.0f,
                      C2D_Color32(116, 230, 152, 255));

    const auto drawTouchButton = [&](const char* label, float y, u32 color) {
        C2D_DrawRectSolid(220.0f, y, 0.2f, 88.0f, 42.0f, C2D_Color32(8, 7, 11, 255));
        C2D_DrawRectSolid(223.0f, y + 3.0f, 0.3f, 82.0f, 36.0f, color);
        const float label_x = std::strlen(label) > 5 ? 228.0f : 239.0f;
        drawText(label, label_x, y + 12.0f, 0.48f, C2D_Color32(245, 245, 245, 255));
    };
    const bool horse_zone = world.zone == Zone::Field || world.zone == Zone::CloudPlateau;
    const char* act_label = horse_zone
                                ? (world.player.mounted ? "DISMOUNT" : "RIDE")
                                : "ACT";
    drawTouchButton(act_label, 58.0f, C2D_Color32(82, 64, 100, 255));
    drawTouchButton("HEAL", 108.0f, C2D_Color32(126, 70, 28, 255));
    const char* utility_label = world.zone == Zone::Field
                                    ? "CALL"
                                    : (world.zone == Zone::CloudPlateau ? "CALL/LOCK" : "LOCK");
    drawTouchButton(utility_label, 158.0f,
                    world.player.lock_on ? C2D_Color32(130, 42, 38, 255)
                                         : C2D_Color32(48, 58, 68, 255));
    C2D_DrawRectSolid(244.0f, 207.0f, 0.2f, 64.0f, 23.0f, C2D_Color32(38, 36, 44, 255));
    drawText("DEBUG", 253.0f, 212.0f, 0.30f, C2D_Color32(170, 170, 178, 255));
    const char* map_legend =
        world.zone == Zone::Field
            ? "white you  brown horse  blue river  red east  cyan west"
        : (world.zone == Zone::CloudPlateau
               ? "white you  brown horse  tan path  red boss"
               : "white: you   gold: objective");
    drawText(map_legend, 16.0f, 207.0f, 0.29f,
             C2D_Color32(165, 165, 172, 255));
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
        C2D_DrawRectSolid(8.0f, 172.0f, 0.7f, 204.0f, 66.0f, C2D_Color32(4, 4, 6, 245));
        drawText(diagnostics, 12.0f, 176.0f, 0.29f, C2D_Color32(116, 230, 152, 255), 196.0f);
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
