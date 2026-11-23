#include <trx/game/output/sources/train_floor.h>

#include <trx/core/utils.h>
#include <trx/game/game_flow.h>
#include <trx/game/matrix.h>
#include <trx/game/output.h>
#include <trx/game/output/mesh_batcher/mesh_builder.h>
#include <trx/game/output/shaders/mesh.h>
#include <trx/game/output/textures.h>
#include <trx/game/rooms/const.h>

// Port of the OG DrawTrainStrips and DrawTrainFloorStrip (specificfx.cpp).

#define M_HALF_LENGTH 20480
#define M_QUAD_SIZE 1024
#define M_WIDE_QUAD_SIZE 3072
#define M_QUAD_COUNT (2 * M_HALF_LENGTH / M_QUAD_SIZE)
#define M_FOG_RAMP_LENGTH 7
#define M_FOG_DEPTH_ADJUSTMENT (-0.005f)
#define M_TEXTURE_COUNT 6

typedef struct {
    int16_t z;
    int16_t near_y;
    int16_t far_y;
    bool wide;
    int8_t texture;
} M_STRIP;

typedef struct {
    MESH_BATCHER *batcher;
    OUTPUT_MESH *mesh;
    OUTPUT_MESH *fog_mesh;
} M_PRIV;

static const M_STRIP m_Strips[] = {
    { .z = -5120, .near_y = 256, .far_y = 256, .wide = true, .texture = 5 },
    { .z = 3072, .near_y = 256, .far_y = 256, .wide = true, .texture = 5 },
    { .z = -2048, .near_y = 256, .far_y = 0, .texture = 3 },
    { .z = 2048, .near_y = 0, .far_y = 256, .texture = 4 },
    { .z = -1024, .near_y = 0, .far_y = 0, .texture = 1 },
    { .z = 1024, .near_y = 0, .far_y = 0, .texture = 2 },
    { .z = 0, .near_y = 0, .far_y = 0, .texture = 0 },
};

static M_PRIV m_Priv = {};

static void M_GetQuadCorners(
    const M_STRIP *const strip, const int32_t quad_idx, XYZW_F corners[4])
{
    const float x0 = -M_HALF_LENGTH + quad_idx * M_QUAD_SIZE;
    const float x1 = x0 + M_QUAD_SIZE;
    const float z0 = strip->z;
    const float z1 = strip->z + (strip->wide ? M_WIDE_QUAD_SIZE : M_QUAD_SIZE);
    corners[0] = (XYZW_F) { .x = x0, .y = strip->near_y, .z = z0 };
    corners[1] = (XYZW_F) { .x = x0, .y = strip->far_y, .z = z1 };
    corners[2] = (XYZW_F) { .x = x1, .y = strip->far_y, .z = z1 };
    corners[3] = (XYZW_F) { .x = x1, .y = strip->near_y, .z = z0 };
}

// Returns how much of the fog color covers a quad of a strip, from 0 to 255.
// The OG fades the seven quads at each end of a strip into the fog with this
// ramp, and leaves the quads between them clear.
static uint8_t M_GetFogAmount(const int32_t quad_idx)
{
    int32_t step;
    if (quad_idx < M_FOG_RAMP_LENGTH) {
        step = quad_idx + 1;
    } else if (quad_idx >= M_QUAD_COUNT - M_FOG_RAMP_LENGTH) {
        step = M_QUAD_COUNT - quad_idx;
    } else {
        return 0;
    }
    return 255 - (step * 16 - 1) * 2;
}

static void M_AddStrip(
    MESH_BUILDER *const builder, const M_STRIP *const strip,
    const int16_t texture_idx)
{
    const SCENE_PASS pass =
        Output_Textures_GetObjectTextureScenePass(texture_idx);
    // The OG overwrites the strips' fog value with the ramp, so the strips
    // take no distance fog.
    uint16_t flags = VERT_NO_FOG | VERT_NO_WIBBLE | VERT_OVERBRIGHT;
    if (pass == SCENE_PASS_OPAQUE) {
        flags |= VERT_NO_ALPHA_DISCARD;
    }

    for (int32_t i = 0; i < M_QUAD_COUNT; i++) {
        XYZW_F corners[4];
        M_GetQuadCorners(strip, i, corners);
        OUTPUT_MESH_VERTEX vertices[4];
        for (int32_t j = 0; j < 4; j++) {
            vertices[j] = (OUTPUT_MESH_VERTEX) {
                .pos = corners[j],
                .flags = flags,
                .uvw_idx = Output_Textures_GetObjectUVWIndex(texture_idx, j),
                .trapezoid_ratio = { 1.0f, 1.0f },
                .reflectivity = 1.0f,
                .shade = SHADE_NEUTRAL,
                .color = { 128, 128, 128, 255 },
            };
        }
        MeshBuilder_AddVertices(builder, vertices, 4);
        MeshBuilder_AddFan(builder, pass, false, true);
    }
}

// Adds white quads over the fogged ends of a strip, with the fog amount as
// their alpha. The instance tint gives them the fog color.
static void M_AddStripFog(
    MESH_BUILDER *const builder, const M_STRIP *const strip)
{
    const uint16_t flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_FOG
        | VERT_NO_WIBBLE | VERT_NO_ALPHA_DISCARD;
    for (int32_t i = 0; i < M_QUAD_COUNT; i++) {
        const uint8_t fog = M_GetFogAmount(i);
        if (fog == 0) {
            continue;
        }
        XYZW_F corners[4];
        M_GetQuadCorners(strip, i, corners);
        OUTPUT_MESH_VERTEX vertices[4];
        for (int32_t j = 0; j < 4; j++) {
            vertices[j] = (OUTPUT_MESH_VERTEX) {
                .pos = corners[j],
                .flags = flags,
                .uvw_idx = -1,
                .trapezoid_ratio = { 1.0f, 1.0f },
                .shade = SHADE_NEUTRAL,
                .color = { 255, 255, 255, fog },
            };
        }
        MeshBuilder_AddVertices(builder, vertices, 4);
        MeshBuilder_AddFan(builder, SCENE_PASS_TRANSPARENT, false, false);
    }
}

static void M_FreeMesh(M_PRIV *const p, OUTPUT_MESH **const mesh)
{
    if (*mesh == nullptr) {
        return;
    }
    MeshBatcher_RemoveMesh(p->batcher, *mesh);
    Output_Mesh_Destroy(*mesh);
    *mesh = nullptr;
}

static void M_FreeMeshes(M_PRIV *const p)
{
    M_FreeMesh(p, &p->mesh);
    M_FreeMesh(p, &p->fog_mesh);
}

static OUTPUT_MESH *M_SealMesh(M_PRIV *const p, MESH_BUILDER *const builder)
{
    OUTPUT_MESH *const mesh = MeshBuilder_Seal(builder);
    MeshBuilder_Destroy(builder);
    if (mesh != nullptr) {
        MeshBatcher_AddMesh(p->batcher, mesh);
    }
    return mesh;
}

static void M_PrepareMesh(M_PRIV *const p)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level == nullptr || level->train_ground == NO_HEIGHT) {
        return;
    }
    const ANIMATED_TEXTURE_RANGE *const range =
        Output_GetAnimatedTextureRange(0);
    if (range == nullptr || range->num_textures < M_TEXTURE_COUNT) {
        return;
    }

    MESH_BUILDER *const builder = MeshBuilder_Create();
    MESH_BUILDER *const fog_builder = MeshBuilder_Create();
    for (size_t i = 0; i < ARRAY_SIZE(m_Strips); i++) {
        const M_STRIP *const strip = &m_Strips[i];
        M_AddStrip(builder, strip, range->textures[strip->texture]);
        M_AddStripFog(fog_builder, strip);
    }
    p->mesh = M_SealMesh(p, builder);
    p->fog_mesh = M_SealMesh(p, fog_builder);
}

void OutputSource_TrainFloor_Init(MESH_BATCHER *const batcher)
{
    m_Priv.batcher = batcher;
}

void OutputSource_TrainFloor_Shutdown(void)
{
    M_FreeMeshes(&m_Priv);
}

void OutputSource_TrainFloor_ObserveLevelLoad(void)
{
    M_PRIV *const p = &m_Priv;
    M_FreeMeshes(p);
    M_PrepareMesh(p);
}

void OutputSource_TrainFloor_ObserveLevelUnload(void)
{
    M_FreeMeshes(&m_Priv);
}

void OutputSource_TrainFloor_Stage(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->mesh == nullptr) {
        return;
    }
    const MESH_INSTANCE inst = {
        .mesh = p->mesh,
        .cwmatrix = *g_MatrixPtr,
        .wmatrix = *g_WMatrixPtr,
        .tint = Output_GetRoomTint(),
    };
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_OPAQUE);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_TRANSPARENT);
    MeshBatcher_Stage(p->batcher, &inst, SCENE_PASS_BLEND_ADD);

    const RGBA_F fog_color = Output_GetFogColor();
    const MESH_INSTANCE fog_inst = {
        .mesh = p->fog_mesh,
        .cwmatrix = *g_MatrixPtr,
        .wmatrix = *g_WMatrixPtr,
        .tint = { fog_color.r, fog_color.g, fog_color.b, 1.0f },
        .depth_adjust = M_FOG_DEPTH_ADJUSTMENT,
    };
    MeshBatcher_Stage(p->batcher, &fog_inst, SCENE_PASS_OPAQUE);
}
