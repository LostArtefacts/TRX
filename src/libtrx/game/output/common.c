#include "game/const.h"
#include "game/matrix.h"
#include "game/output.h"
#include "utils.h"

#define MAX_DYNAMIC_LIGHTS 10

typedef struct {
    XYZ_32 pos;
    int32_t shade;
} COMMON_LIGHT;

static int32_t m_FogStart = 0;
static int32_t m_DynamicLightCount = 0;
static LIGHT m_DynamicLights[MAX_DYNAMIC_LIGHTS] = {};

static void M_CalculateBrightestLight(
    XYZ_32 pos, const ROOM *room, COMMON_LIGHT *brightest_light);
static int32_t M_CalculateDynamicLight(
    XYZ_32 pos, COMMON_LIGHT *brightest_light);

static void M_CalculateBrightestLight(
    const XYZ_32 pos, const ROOM *const room,
    COMMON_LIGHT *const brightest_light)
{
#if TR_VERSION == 2
    if (room->light_mode != RLM_NORMAL) {
        const int32_t light_shade = Output_GetRoomLightShade(room->light_mode);
        for (int32_t i = 0; i < room->num_lights; i++) {
            const LIGHT *const light = &room->lights[i];
            const int32_t dx = pos.x - light->pos.x;
            const int32_t dy = pos.y - light->pos.y;
            const int32_t dz = pos.z - light->pos.z;

            const int32_t falloff_1 = SQUARE(light->falloff.value_1) >> 12;
            const int32_t falloff_2 = SQUARE(light->falloff.value_2) >> 12;
            const int32_t dist = (SQUARE(dx) + SQUARE(dy) + SQUARE(dz)) >> 12;

            const int32_t shade_1 =
                falloff_1 * light->shade.value_1 / (falloff_1 + dist);
            const int32_t shade_2 =
                falloff_2 * light->shade.value_2 / (falloff_2 + dist);
            const int32_t shade =
                shade_1 + (shade_2 - shade_1) * light_shade / (WIBBLE_SIZE - 1);

            if (shade > brightest_light->shade) {
                brightest_light->shade = shade;
                brightest_light->pos = light->pos;
            }
        }
        return;
    }
#endif

    const int32_t ambient = TR_VERSION == 1 ? (SHADE_MAX - room->ambient) : 0;
    for (int32_t i = 0; i < room->num_lights; i++) {
        const LIGHT *const light = &room->lights[i];
        const int32_t dx = pos.x - light->pos.x;
        const int32_t dy = pos.y - light->pos.y;
        const int32_t dz = pos.z - light->pos.z;
        const int32_t falloff = SQUARE(light->falloff.value_1) >> 12;
        const int32_t dist = (SQUARE(dx) + SQUARE(dy) + SQUARE(dz)) >> 12;
        const int32_t shade =
            ambient + (falloff * light->shade.value_1 / (falloff + dist));
        if (shade > brightest_light->shade) {
            brightest_light->shade = shade;
            brightest_light->pos = light->pos;
        }
    }
}

static int32_t M_CalculateDynamicLight(
    const XYZ_32 pos, COMMON_LIGHT *const brightest_light)
{
    int32_t adder = 0;
    for (int32_t i = 0; i < m_DynamicLightCount; i++) {
        const LIGHT *const light = &m_DynamicLights[i];
        const int32_t dx = pos.x - light->pos.x;
        const int32_t dy = pos.y - light->pos.y;
        const int32_t dz = pos.z - light->pos.z;
        const int32_t radius = 1 << light->falloff.value_1;
        if (dx < -radius || dx > radius || dy < -radius || dy > radius
            || dz < -radius || dz > radius) {
            continue;
        }

        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist > SQUARE(radius)) {
            continue;
        }

        const int32_t shade = (1 << light->shade.value_1)
            - (dist >> (2 * light->falloff.value_1 - light->shade.value_1));
        if (shade > brightest_light->shade) {
            brightest_light->shade = shade;
            brightest_light->pos = light->pos;
        }
        adder += shade;
    }

    return adder;
}

void Output_CalculateLight(const XYZ_32 pos, const int16_t room_num)
{
    const ROOM *const room = Room_Get(room_num);
    COMMON_LIGHT brightest_light = {};

    M_CalculateBrightestLight(pos, room, &brightest_light);
    int32_t adder = brightest_light.shade;
    int32_t dynamic_adder = M_CalculateDynamicLight(pos, &brightest_light);

    adder = (adder + dynamic_adder) / 2;
    if (TR_VERSION == 1 && (room->num_lights > 0 || dynamic_adder > 0)) {
        adder += (SHADE_MAX - room->ambient) / 2;
    }

    // TODO: use m_LsAdder and m_LsDivider once ported
    int32_t global_adder;
    int32_t global_divider;
    if (adder == 0) {
        global_adder = room->ambient;
        global_divider = 0;
    } else {
#if TR_VERSION == 1
        global_adder = SHADE_MAX - adder;
        const int32_t divider = brightest_light.shade == adder
            ? adder
            : brightest_light.shade - adder;
        global_divider = (1 << (W2V_SHIFT + 12)) / divider;
#else
        global_adder = room->ambient - adder;
        global_divider = (1 << (W2V_SHIFT + 12)) / adder;
#endif
        int16_t angles[2];
        Math_GetVectorAngles(
            pos.x - brightest_light.pos.x, pos.y - brightest_light.pos.y,
            pos.z - brightest_light.pos.z, angles);
        Output_RotateLight(angles[1], angles[0]);
    }

    const int32_t depth = g_MatrixPtr->_23 >> W2V_SHIFT;
    global_adder += Output_CalcFogShade(depth);
    CLAMPG(global_adder, SHADE_MAX);

    Output_SetLightAdder(global_adder);
    Output_SetLightDivider(global_divider);
}

void Output_CalculateStaticLight(const int16_t adder)
{
    // TODO: use m_LsAdder
    int32_t global_adder = adder - SHADE_NEUTRAL;
    const int32_t depth = g_MatrixPtr->_23 >> W2V_SHIFT;
    global_adder += Output_CalcFogShade(depth);
    CLAMPG(global_adder, SHADE_MAX);
    Output_SetLightAdder(global_adder);
}

void Output_CalculateStaticMeshLight(
    const XYZ_32 pos, const SHADE shade, const ROOM *const room)
{
    int32_t adder = shade.value_1;
    if (TR_VERSION == 2 && room->light_mode != RLM_NORMAL) {
        const int32_t room_shade = Output_GetRoomLightShade(room->light_mode);
        adder +=
            (shade.value_2 - shade.value_1) * room_shade / (WIBBLE_SIZE - 1);
    }

    for (int32_t i = 0; i < m_DynamicLightCount; i++) {
        const LIGHT *const light = &m_DynamicLights[i];
        const int32_t dx = pos.x - light->pos.x;
        const int32_t dy = pos.y - light->pos.y;
        const int32_t dz = pos.z - light->pos.z;
        const int32_t radius = 1 << light->falloff.value_1;
        if (dx < -radius || dx > radius || dy < -radius || dy > radius
            || dz < -radius || dz > radius) {
            continue;
        }

        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist > SQUARE(radius)) {
            continue;
        }

        const int32_t shade = (1 << light->shade.value_1)
            - (dist >> (2 * light->falloff.value_1 - light->shade.value_1));
        adder -= shade;
        if (adder < 0) {
            break;
        }
    }

    Output_CalculateStaticLight(adder);
}

void Output_CalculateObjectLighting(
    const ITEM *const item, const BOUNDS_16 *const bounds)
{
    if (item->shade.value_1 >= 0) {
        Output_CalculateStaticMeshLight(
            item->pos, item->shade, Room_Get(item->room_num));
        return;
    }

    Matrix_PushUnit();

    Matrix_TranslateSet(0, 0, 0);
    Matrix_Rot16(item->rot);
    Matrix_TranslateRel32((XYZ_32) {
        .x = (bounds->min.x + bounds->max.x) / 2,
        .y = (bounds->max.y + bounds->min.y) / 2,
        .z = (bounds->max.z + bounds->min.z) / 2,
    });
    const XYZ_32 pos = {
        .x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT),
        .y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT),
        .z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT),
    };
    Matrix_Pop();

    Output_CalculateLight(pos, item->room_num);
}

void Output_LightRoom(ROOM *const room)
{
    if (TR_VERSION == 2 && room->light_mode != RLM_NORMAL) {
        Output_LightRoomVertices(room);
    } else if (room->flags & RF_DYNAMIC_LIT) {
        for (int32_t i = 0; i < room->mesh.num_vertices; i++) {
            ROOM_VERTEX *const vtx = &room->mesh.vertices[i];
            vtx->light_adder = vtx->light_base;
        }
        room->flags &= ~RF_DYNAMIC_LIT;
    }

    const int32_t x_min = WALL_L;
    const int32_t z_min = WALL_L;
    const int32_t x_max = (room->size.x - 1) * WALL_L;
    const int32_t z_max = (room->size.z - 1) * WALL_L;

    for (int32_t i = 0; i < m_DynamicLightCount; i++) {
        const LIGHT *const light = &m_DynamicLights[i];
        const int32_t x = light->pos.x - room->pos.x;
        const int32_t y = light->pos.y;
        const int32_t z = light->pos.z - room->pos.z;
        const int32_t radius = 1 << light->falloff.value_1;
        if (x - radius > x_max || z - radius > z_max || x + radius < x_min
            || z + radius < z_min) {
            continue;
        }

        room->flags |= RF_DYNAMIC_LIT;

        for (int32_t j = 0; j < room->mesh.num_vertices; j++) {
            ROOM_VERTEX *const v = &room->mesh.vertices[j];
            if (v->light_adder == 0) {
                continue;
            }

            const int32_t dx = v->pos.x - x;
            const int32_t dy = v->pos.y - y;
            const int32_t dz = v->pos.z - z;
            if (dx < -radius || dx > radius || dy < -radius || dy > radius
                || dz < -radius || dz > radius) {
                continue;
            }

            const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
            if (dist > SQUARE(radius)) {
                continue;
            }

            const int32_t shade = (1 << light->shade.value_1)
                - (dist >> (2 * light->falloff.value_1 - light->shade.value_1));
            v->light_adder -= shade;
            CLAMPL(v->light_adder, 0);
        }
    }
}

void Output_ResetDynamicLights(void)
{
    m_DynamicLightCount = 0;
}

void Output_AddDynamicLight(
    const XYZ_32 pos, const int32_t intensity, const int32_t falloff)
{
    const int32_t idx =
        m_DynamicLightCount < MAX_DYNAMIC_LIGHTS ? m_DynamicLightCount++ : 0;

    LIGHT *const light = &m_DynamicLights[idx];
    light->pos = pos;
    light->shade.value_1 = intensity;
    light->falloff.value_1 = falloff;
}

int32_t Output_GetFogStart(void)
{
    return MIN(m_FogStart, Output_GetFogEnd());
}

void Output_SetFogStart(const int32_t dist)
{
    m_FogStart = dist;
}
