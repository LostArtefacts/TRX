#include <trx/game/output/train_floor.h>

#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/game_flow.h>
#include <trx/game/interpolation.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/draw.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/objects.h>
#include <trx/game/output/sources/train_floor.h>
#include <trx/game/rooms/const.h>

// Port of the OG DrawTrainFloor and DrawTrainObjects (train.cpp).

#define M_TRACK_Z 52224
#define M_NEAR_ROW_Z 47168
#define M_FAR_ROW_Z 58304
#define M_FAR_ROW_ROT 32760
#define M_STRIP_SNAP 1024
#define M_TILE 6144
#define M_ROW_LENGTH 9
#define M_ROW_START (-4 * M_TILE)
#define M_MAP_WRAP (64 * M_TILE)
#define M_MAP_SPEED_SHIFT 5
#define M_MAP_MASK 0x1F
#define M_NO_ROCK (-1)

typedef struct {
    int16_t static_id;
    int16_t z_offset;
} M_ROCK;

// The OG dels_handy_train_map: architecture statics 6 to 9.
static const int16_t m_Cliffs[128] = {
    36, 36, 36, 37, 38, 39, 36, 37, 38, 38, 38, 39, 37, 38, 39, 36, //
    36, 36, 36, 36, 37, 39, 36, 36, 37, 38, 38, 39, 36, 36, 37, 39, //
    36, 36, 36, 37, 38, 39, 36, 37, 38, 38, 38, 39, 37, 38, 39, 36, //
    36, 36, 36, 36, 37, 39, 36, 36, 37, 38, 38, 39, 36, 36, 37, 39, //
    38, 37, 36, 39, 37, 36, 36, 36, 39, 38, 38, 38, 38, 37, 36, 39, //
    38, 38, 37, 39, 38, 38, 38, 38, 38, 38, 37, 36, 36, 36, 39, 38, //
    38, 37, 36, 39, 37, 36, 36, 36, 39, 38, 38, 38, 38, 37, 36, 39, //
    38, 38, 37, 39, 38, 38, 38, 38, 38, 38, 37, 36, 36, 36, 39, 38, //
};

// The OG dels_handy_train_map2: rock statics 0, 2 and 3.
static const M_ROCK m_Rocks[64] = {
    { 22, 4096 },     { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { 20, -3072 },
    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { 20, 4096 },
    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 },
    { M_NO_ROCK, 0 }, { 20, -3072 },    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 },
    { M_NO_ROCK, 0 }, { 20, 4096 },     { M_NO_ROCK, 0 }, { 23, -3072 },
    { M_NO_ROCK, 0 }, { 20, -3072 },    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 },
    { 20, 4096 },     { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { 20, 4096 },
    { 23, -2048 },    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 },
    { 22, 4096 },     { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { 20, -3072 },
    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { 20, 4096 },
    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 },
    { M_NO_ROCK, 0 }, { 20, -3072 },    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 },
    { M_NO_ROCK, 0 }, { 20, 4096 },     { M_NO_ROCK, 0 }, { 23, -3072 },
    { M_NO_ROCK, 0 }, { 20, -3072 },    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 },
    { 20, 4096 },     { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { 20, 4096 },
    { 23, -2048 },    { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 }, { M_NO_ROCK, 0 },
};

static const int16_t m_SceneryStatics[] = { 20, 22, 23, 36, 37, 38, 39 };

static int32_t m_MapPos = 0;
static int32_t m_MapPosPrev = 0;

static bool M_IsEnabled(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    return level != nullptr && level->train_ground != NO_HEIGHT;
}

static int32_t M_GetInterpolatedMapPos(void)
{
    int32_t delta = m_MapPos - m_MapPosPrev;
    if (delta > M_MAP_WRAP / 2) {
        delta -= M_MAP_WRAP;
    } else if (delta < -M_MAP_WRAP / 2) {
        delta += M_MAP_WRAP;
    }
    const int32_t pos =
        m_MapPos - delta + (int32_t)(delta * Interpolation_GetWorldRate());
    return (pos % M_MAP_WRAP + M_MAP_WRAP) % M_MAP_WRAP;
}

// Lights the scenery to full brightness without the mesh prelight, as the OG
// phd_PutPolygons_train does.
static bool M_GetVertexShade(
    const FACE *const face, const int32_t vertex_idx, int32_t *const shade)
{
    *shade = 0;
    return true;
}

static void M_DrawStatic(
    const int16_t static_id, const XYZ_32 pos, const int16_t rot_y)
{
    const STATIC_OBJECT_3D *const obj = Object_Get3DStatic(static_id);
    if (!obj->loaded) {
        return;
    }

    Matrix_Push();
    Matrix_TranslateAbs32(pos);
    Matrix_RotY(rot_y);
    const CLIP clip = Output_CheckBoundsClip(&obj->draw_bounds);
    if (clip != CLIP_NOT_VISIBLE) {
        const OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx);
        // A prelit mesh reads the ambient as a multiplier over the full
        // shade, and a mesh with normals on the 128-neutral scale.
        const float ambient = mesh->num_lights > 0 ? 1.0f : 128.0f / 255.0f;
        Output_CalculateStaticLightRGB_F((RGB_F) { ambient, ambient, ambient });
        Object_DrawMesh(obj->mesh_idx, clip, false);
    }
    Matrix_Pop();
}

static OUTPUT_OBJECT_MESH_POLICY m_SceneryMeshPolicy = {
    .get_vertex_shade = M_GetVertexShade,
};

void Output_TrainFloor_ObserveLevelLoad(void)
{
    m_MapPos = 0;
    m_MapPosPrev = 0;
    if (!M_IsEnabled()) {
        return;
    }
    for (size_t i = 0; i < ARRAY_SIZE(m_SceneryStatics); i++) {
        const STATIC_OBJECT_3D *const obj =
            Object_Get3DStatic(m_SceneryStatics[i]);
        if (obj->loaded) {
            OutputSource_Objects_AddMeshPolicy(
                obj->mesh_idx, &m_SceneryMeshPolicy);
        }
    }
}

void Output_TrainFloor_ObserveLevelUnload(void)
{
    OutputSource_Objects_RemoveMeshPolicy(&m_SceneryMeshPolicy);
}

void Output_TrainFloor_Update(void)
{
    m_MapPosPrev = m_MapPos;
    if (!M_IsEnabled()) {
        return;
    }
    const int32_t step = Output_GetUVRotateSpeed() << M_MAP_SPEED_SHIFT;
    m_MapPos = ((m_MapPos + step) % M_MAP_WRAP + M_MAP_WRAP) % M_MAP_WRAP;
}

int32_t Output_TrainFloor_GetMapPos(void)
{
    return m_MapPos;
}

void Output_TrainFloor_SetMapPos(const int32_t pos)
{
    m_MapPos = (pos % M_MAP_WRAP + M_MAP_WRAP) % M_MAP_WRAP;
    m_MapPosPrev = m_MapPos;
}

void Output_TrainFloor_Draw(void)
{
    if (!M_IsEnabled()) {
        return;
    }

    const int32_t ground = GF_GetCurrentLevel()->train_ground;
    const int32_t view_x = g_ViewPos.x;

    Matrix_Push();
    Matrix_TranslateAbs(view_x & -M_STRIP_SNAP, ground, M_TRACK_Z);
    OutputSource_TrainFloor_Stage();
    Matrix_Pop();

    const int32_t map_pos = M_GetInterpolatedMapPos();
    const int32_t map_idx = map_pos / M_TILE - view_x / M_TILE;
    const int32_t start_x =
        view_x / M_TILE * M_TILE + map_pos % M_TILE + M_ROW_START;
    const int32_t y = ground + STEP_L;
    const int32_t near_idx = 96 - (map_idx & M_MAP_MASK);
    const int32_t far_idx = 32 - ((map_idx + 8) & M_MAP_MASK);
    const int32_t rock_idx = 32 - (map_idx & M_MAP_MASK);
    for (int32_t i = 0; i < M_ROW_LENGTH; i++) {
        const int32_t x = start_x + i * M_TILE;
        M_DrawStatic(
            m_Cliffs[near_idx + i], (XYZ_32) { x, y, M_NEAR_ROW_Z }, 0);
        M_DrawStatic(
            m_Cliffs[far_idx + i], (XYZ_32) { x, y, M_FAR_ROW_Z },
            M_FAR_ROW_ROT);
        const M_ROCK *const rock = &m_Rocks[rock_idx + i];
        if (rock->static_id != M_NO_ROCK) {
            M_DrawStatic(
                rock->static_id, (XYZ_32) { x, y, M_TRACK_Z + rock->z_offset },
                0);
        }
    }
}

bool Output_TrainFloor_IsEnabled(void)
{
    return M_IsEnabled();
}
