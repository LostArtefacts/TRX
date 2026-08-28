#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/output/state.h>
#include <trx/game/output/water/priv.h>
#include <trx/game/rooms.h>

static RGBA_F M_GetWaterTint(void)
{
    return g_Camera.underwater ? Output_GetTint()
                               : Color_RGBToRGBA(Output_GetWaterColor());
}

// Selects a mesh by its origin, as TR3 does because underwater views already
// carry the water colour.
static RGBA_F M_GetPerMeshTint(const GAME_VECTOR pos)
{
    if (g_Camera.underwater) {
        return Output_GetTint();
    }

    int16_t room_num = pos.room_num;
    Room_GetSector(pos.pos, &room_num);
    if (!Room_Get(room_num)->flags.underwater) {
        return COLOR_RGBA_F_WHITE;
    }
    const int32_t water_height = Room_GetWaterHeight(pos.pos, room_num);
    if (water_height == NO_HEIGHT || pos.y > water_height) {
        return M_GetWaterTint();
    }
    return COLOR_RGBA_F_WHITE;
}

// Returns the world-space water surface height above a point, or NO_HEIGHT
// when no surface exists.
static int32_t M_ProbeWaterSurface(const GAME_VECTOR pos)
{
    int16_t room_num = pos.room_num;
    Room_GetSector(pos.pos, &room_num);
    return Room_GetWaterHeight(pos.pos, room_num);
}

// Finds water reached by a mesh by testing its lowest possible position first,
// including meshes whose centres remain above water.
static int32_t M_FindMeshWaterSurface(
    const GAME_VECTOR pos, const int32_t radius)
{
    GAME_VECTOR deepest = pos;
    deepest.pos.y += radius;
    const int32_t height = M_ProbeWaterSurface(deepest);
    if (height != NO_HEIGHT) {
        return height;
    }
    return M_ProbeWaterSurface(pos);
}

static WATER_EFFECTS M_GetEffects(
    const bool is_below_water, const bool is_camera_underwater)
{
    if (is_below_water) {
        return (WATER_EFFECTS) {
            .shade = true,
            .room_shade = true,
            .wibble = !is_camera_underwater,
        };
    }
    return (WATER_EFFECTS) {
        .shade = is_camera_underwater,
        .room_shade = is_camera_underwater,
        .wibble = is_camera_underwater,
    };
}

static bool M_IsObjectWibbleEnabled(void)
{
    return false;
}

static WATER_LARA_MESH M_DecideLaraMesh(
    const LARA_MESH mesh, const GAME_VECTOR pos, const int32_t radius)
{
    switch (g_Config.visuals.water_tint_mode) {
    case WATER_TINT_MODE_WHOLE:
        return (WATER_LARA_MESH) { .tint = Output_GetTint() };

    case WATER_TINT_MODE_PER_MESH:
        return (WATER_LARA_MESH) { .tint = M_GetPerMeshTint(pos) };

    case WATER_TINT_MODE_RESPONSIVE: {
        const int32_t surface = M_FindMeshWaterSurface(pos, radius);
        if (surface == NO_HEIGHT) {
            return (WATER_LARA_MESH) { .tint = COLOR_RGBA_F_WHITE };
        }
        return (WATER_LARA_MESH) {
            .tint = M_GetWaterTint(),
            .has_surface = true,
            .surface = surface,
        };
    }

    case WATER_TINT_MODE_NUMBER_OF:
        break;
    }
    ASSERT_FAIL();
    return (WATER_LARA_MESH) { .tint = COLOR_RGBA_F_WHITE };
}

const WATER_MODEL g_WaterModelTR123 = {
    .get_effects = M_GetEffects,
    .is_object_wibble_enabled = M_IsObjectWibbleEnabled,
    .decide_lara_mesh = M_DecideLaraMesh,
};
