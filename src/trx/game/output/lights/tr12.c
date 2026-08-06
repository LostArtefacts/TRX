#include <trx/core/colors.h>
#include <trx/core/math/geom.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/const.h>
#include <trx/game/output.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/lights/fog_bulbs.h>
#include <trx/game/output/lights/priv.h>
#include <trx/gl/utils.h>
#include <trx/version.h>

#include <string.h>

// The greatest falloff exponent the shading loop's shift stays sane for.
#define M_MAX_FALLOFF_EXP 15

typedef struct {
    XYZ_32 pos;
    int32_t shade;
} M_COMMON_LIGHT;

static void M_CalculateBrightestLight(
    const XYZ_32 pos, const ROOM *const room,
    M_COMMON_LIGHT *const brightest_light)
{
    if (room->light_mode != RLM_NORMAL) {
        const int32_t light_shade = Output_GetRoomLightShade(room->light_mode);
        for (int32_t i = 0; i < room->num_lights; i++) {
            const LIGHT *const light = &room->lights[i];
            if (light->layout != LIGHT_LAYOUT_LEGACY) {
                continue;
            }
            const LIGHT_LEGACY_DATA *const data =
                Output_Lights_GetLegacyData(light);
            const int32_t dx = pos.x - light->pos.x;
            const int32_t dy = pos.y - light->pos.y;
            const int32_t dz = pos.z - light->pos.z;

            const int32_t falloff_1 = SQUARE(data->falloff.value_1) >> 12;
            const int32_t falloff_2 = SQUARE(data->falloff.value_2) >> 12;
            const int32_t dist = (SQUARE(dx) + SQUARE(dy) + SQUARE(dz)) >> 12;

            const int32_t shade_1 =
                falloff_1 * data->shade.value_1 / MAX(1, falloff_1 + dist);
            const int32_t shade_2 =
                falloff_2 * data->shade.value_2 / MAX(1, falloff_2 + dist);
            const int32_t shade = shade_1
                + (shade_2 - shade_1) * light_shade / (OUTPUT_LIGHT_CYCLE - 1);

            if (shade > brightest_light->shade) {
                brightest_light->shade = shade;
                brightest_light->pos = light->pos;
            }
        }
        return;
    }

    const int32_t ambient = g_TRVersion == 1 ? (SHADE_MAX - room->ambient) : 0;
    for (int32_t i = 0; i < room->num_lights; i++) {
        const LIGHT *const light = &room->lights[i];
        if (light->layout != LIGHT_LAYOUT_LEGACY) {
            continue;
        }
        const LIGHT_LEGACY_DATA *const data =
            Output_Lights_GetLegacyData(light);
        const int32_t dx = pos.x - light->pos.x;
        const int32_t dy = pos.y - light->pos.y;
        const int32_t dz = pos.z - light->pos.z;
        const int32_t falloff = SQUARE(data->falloff.value_1) >> 12;
        const int32_t dist = (SQUARE(dx) + SQUARE(dy) + SQUARE(dz)) >> 12;
        const int32_t shade =
            ambient + (falloff * data->shade.value_1 / (falloff + dist));
        if (shade > brightest_light->shade) {
            brightest_light->shade = shade;
            brightest_light->pos = light->pos;
        }
    }
}

static int32_t M_CalculateDynamicLight(
    const XYZ_32 pos, M_COMMON_LIGHT *const brightest_light)
{
    int32_t adder = 0;
    VECTOR *const dynamic_lights = Output_GetDynamicLights();
    for (int32_t i = 0; i < dynamic_lights->count; i++) {
        const OUTPUT_DYNAMIC_LIGHT *const entry = Vector_Get(dynamic_lights, i);
        const LIGHT *const light = &entry->light;
        const LIGHT_LEGACY_DATA *const data =
            Output_Lights_GetLegacyData(light);
        const int32_t dx = pos.x - light->pos.x;
        const int32_t dy = pos.y - light->pos.y;
        const int32_t dz = pos.z - light->pos.z;
        const int32_t radius = 1 << data->falloff.value_1;
        if (dx < -radius || dx > radius || dy < -radius || dy > radius
            || dz < -radius || dz > radius) {
            continue;
        }

        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist > SQUARE(radius)) {
            continue;
        }

        const int32_t shade = (1 << data->shade.value_1)
            - (dist >> (2 * data->falloff.value_1 - data->shade.value_1));
        if (shade > brightest_light->shade) {
            brightest_light->shade = shade;
            brightest_light->pos = light->pos;
        }
        adder += shade;
    }

    return adder;
}

static void M_CalculateLight(const XYZ_32 pos, const int16_t room_num)
{
    const ROOM *const room = Room_Get(room_num);

    M_COMMON_LIGHT brightest_light = {};

    M_CalculateBrightestLight(pos, room, &brightest_light);
    int32_t adder = brightest_light.shade;
    int32_t dynamic_adder = M_CalculateDynamicLight(pos, &brightest_light);

    adder = (adder + dynamic_adder) / 2;
    if (g_TRVersion == 1 && (room->num_lights > 0 || dynamic_adder > 0)) {
        adder += (SHADE_MAX - room->ambient) / 2;
    }

    // TODO: use m_LsAdder and m_LsDivider once ported
    int32_t global_adder;
    int32_t global_divider;
    if (adder == 0) {
        global_adder = room->ambient;
        global_divider = 0;
    } else {
        if (g_TRVersion == 1) {
            global_adder = SHADE_MAX - adder;
            const int32_t divider = brightest_light.shade == adder
                ? adder
                : brightest_light.shade - adder;
            global_divider = (1 << W2V_SHIFT) * SHADE_NEUTRAL / divider;
        } else {
            global_adder = room->ambient - adder;
            global_divider = (1 << W2V_SHIFT) * SHADE_NEUTRAL / adder;
        }
        int16_t angles[2];
        Math_GetVectorAngles(
            pos.x - brightest_light.pos.x, pos.y - brightest_light.pos.y,
            pos.z - brightest_light.pos.z, angles);
        Output_RotateLight(angles[1], angles[0]);
    }

    CLAMPG(global_adder, SHADE_MAX);

    Output_SetLightAdder(global_adder);
    Output_SetLightDivider(global_divider);
}

static void M_CalculateObjectLightingAt(
    const ITEM *const item, const GAME_VECTOR sample_pos)
{
    M_CalculateLight(sample_pos.pos, sample_pos.room_num);
}

static void M_CalculateStaticLight(const int16_t adder)
{
    Output_Lights_SetScalarStaticLight(adder);
}

static void M_CalculateStaticMeshLight(
    const XYZ_32 pos, const SHADE shade, const ROOM *const room)
{
    int32_t adder = shade.value_1;
    if (room->light_mode != RLM_NORMAL) {
        const int32_t room_shade = Output_GetRoomLightShade(room->light_mode);
        adder += (shade.value_2 - shade.value_1) * room_shade
            / (OUTPUT_LIGHT_CYCLE - 1);
    }

    VECTOR *const dynamic_lights = Output_GetDynamicLights();
    for (int32_t i = 0; i < dynamic_lights->count; i++) {
        const OUTPUT_DYNAMIC_LIGHT *const entry = Vector_Get(dynamic_lights, i);
        const LIGHT *const light = &entry->light;
        const LIGHT_LEGACY_DATA *const data =
            Output_Lights_GetLegacyData(light);
        const int32_t dx = pos.x - light->pos.x;
        const int32_t dy = pos.y - light->pos.y;
        const int32_t dz = pos.z - light->pos.z;
        const int32_t radius = 1 << data->falloff.value_1;
        if (dx < -radius || dx > radius || dy < -radius || dy > radius
            || dz < -radius || dz > radius) {
            continue;
        }

        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist > SQUARE(radius)) {
            continue;
        }

        adder -= (1 << data->shade.value_1)
            - (dist >> (2 * data->falloff.value_1 - data->shade.value_1));
        if (adder < 0) {
            break;
        }
    }

    M_CalculateStaticLight(adder);
}

static void M_AddDynamicLight(
    const XYZ_32 pos, const int32_t intensity, const int32_t falloff)
{
    const OUTPUT_DYNAMIC_LIGHT light = {
        .light = {
            .pos = pos,
            .color = COLOR_RGB_888_WHITE,
            .layout = LIGHT_LAYOUT_LEGACY,
            .type = LIGHT_TYPE_POINT,
            .u.legacy =
                {
                    .shade.value_1 = intensity,
                    .falloff.value_1 = falloff,
                },
        },
        .kind = OUTPUT_DYNAMIC_LIGHT_LUM,
    };
    Vector_Add(Output_GetDynamicLights(), &light);
}

static int32_t M_Log2(int32_t value)
{
    int32_t exponent = 0;
    while (value > 1) {
        value >>= 1;
        exponent++;
    }
    return exponent;
}

// TR1/2 shade in luminance and measure a light by the exponents the OG shading
// loop shifts by, where the colored entry point gives a radius and a color.
// This is the conversion Output_Lights_TR3_AddDynamicLight makes, taken the
// other way: the brightest channel stands for the light, and its color is lost.
static void M_AddDynamicLightRGB(
    const XYZ_32 pos, const int32_t falloff, const RGB_888 color)
{
    int32_t safe_falloff = falloff;
    CLAMP(safe_falloff, 1, OUTPUT_DYNAMIC_FALLOFF_MAX);
    const int32_t radius = safe_falloff << OUTPUT_DYNAMIC_RADIUS_SHIFT;

    int32_t shade = MAX3(color.r, color.g, color.b) << 4;
    CLAMPL(shade, 1);

    int32_t falloff_exp = M_Log2(radius);
    CLAMPG(falloff_exp, M_MAX_FALLOFF_EXP);

    M_AddDynamicLight(pos, M_Log2(shade), falloff_exp);
}

static void M_UploadCPULight(
    const OUTPUT_UNIFORMS *const uniforms, const OUTPUT_LIGHT_INFO *const info)
{
    OUTPUT_LS_CACHE *const cache = Output_Lights_GetLSCache();
    if (cache->mode == LS_MODE_FULL
        && memcmp(&cache->last_info, info, sizeof(*info)) == 0) {
        return;
    }
    memcpy(&cache->last_info, info, sizeof(*info));
    cache->last_own_adder = info->ls_adder;
    cache->last_own_tr3_ambient = info->tr3_ambient;
    cache->mode = LS_MODE_FULL;

    OUTPUT_UNIFORM_LS_TR12 ls = {};
    ls.adder = info->ls_adder;
    ls.divider = info->ls_divider / (float)(1 << (W2V_SHIFT));
    ls.vector_view[0] = info->ls_vector_view.x;
    ls.vector_view[1] = info->ls_vector_view.y;
    ls.vector_view[2] = info->ls_vector_view.z;
    ls.vector_view[3] = 0;

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->ls);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(ls), &ls);
    TRX_GL_CheckError();
}

static void M_UploadOwnLight(
    const OUTPUT_UNIFORMS *const uniforms, const OUTPUT_LIGHT_INFO *const info)
{
    OUTPUT_LS_CACHE *const cache = Output_Lights_GetLSCache();
    if (cache->mode == LS_MODE_OWN && cache->last_own_adder == info->ls_adder) {
        return;
    }

    const float light_adder = info->ls_adder;
    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->ls);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER,
        offsetof(OUTPUT_UNIFORM_LS_TR12, adder), sizeof(light_adder),
        &light_adder);
    cache->last_own_adder = info->ls_adder;
    cache->mode = LS_MODE_OWN;
}

const LIGHTING_MODEL g_LightingModelTR12 = {
    .calculate_light = M_CalculateLight,
    .calculate_object_lighting_at = M_CalculateObjectLightingAt,
    .calculate_static_light = M_CalculateStaticLight,
    .calculate_static_mesh_light = M_CalculateStaticMeshLight,
    .add_dynamic_light = M_AddDynamicLight,
    .add_dynamic_light_rgb = M_AddDynamicLightRGB,
    .upload_cpu_light = M_UploadCPULight,
    .upload_own_light = M_UploadOwnLight,
    .prepare_scene = Output_FogBulbs_PrepareScene,
    .shader_variant = 0,
};
