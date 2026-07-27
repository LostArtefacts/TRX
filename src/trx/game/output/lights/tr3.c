#include <trx/core/colors.h>
#include <trx/core/math/geom.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/const.h>
#include <trx/game/items/manager.h>
#include <trx/game/output.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/lights/priv.h>
#include <trx/gl/utils.h>
#include <trx/version.h>

#include <math.h>
#include <string.h>

#define M_A_SHIFT 5

typedef struct {
    XYZ_32 sun_dir_world;
    XYZ_32 bulb_dir_world;
    XYZ_32 dynamic_dir_world;
    RGB_888 sun_color;
    RGB_888 bulb_color;
    RGB_888 dynamic_color;
    uint8_t ambient;
    struct {
        bool has_sun : 1;
        bool has_bulb : 1;
        bool has_dynamic : 1;
        bool has_ambient : 1;
    } flags;
} M_ITEM_LIGHT;

static M_ITEM_LIGHT m_ItemLights[MAX_ITEMS] = {};

static RGB_F M_RGB15ToRGBF(const int16_t rgb15)
{
    const int32_t r8 = (rgb15 & 0x1F) << 3;
    const int32_t g8 = (rgb15 & 0x3E0) >> 2;
    const int32_t b8 = (rgb15 & 0x7C00) >> 7;
    return (RGB_F) {
        .r = r8 / 255.0f,
        .g = g8 / 255.0f,
        .b = b8 / 255.0f,
    };
}

static uint8_t M_LerpU8Shift(
    const uint8_t current, const uint8_t target, const int32_t shift)
{
    const int32_t cur = (int32_t)current;
    const int32_t dst = (int32_t)target;
    int32_t next = cur + ((dst - cur) >> shift);
    CLAMP(next, 0, 255);
    return (uint8_t)next;
}

static RGB_888 M_LerpRGBShift(
    const RGB_888 current, const RGB_888 target, const int32_t shift)
{
    return (RGB_888) {
        .r = M_LerpU8Shift(current.r, target.r, shift),
        .g = M_LerpU8Shift(current.g, target.g, shift),
        .b = M_LerpU8Shift(current.b, target.b, shift),
    };
}

static XYZ_32 M_LerpXYZShift(
    const XYZ_32 current, const XYZ_32 target, const int32_t shift)
{
    return (XYZ_32) {
        .x = current.x + ((target.x - current.x) >> shift),
        .y = current.y + ((target.y - current.y) >> shift),
        .z = current.z + ((target.z - current.z) >> shift),
    };
}

static XYZ_32 M_NormalizeDeltaWorld(const XYZ_32 delta)
{
    const int32_t dx = delta.x >> 2;
    const int32_t dy = delta.y >> 2;
    const int32_t dz = delta.z >> 2;
    const uint32_t len =
        Math_Sqrt((uint32_t)(SQUARE(dx) + SQUARE(dy) + SQUARE(dz)));
    if (len == 0u) {
        return (XYZ_32) { 0, 0, 0 };
    }

    return (XYZ_32) {
        .x = (dx * (1 << W2V_SHIFT)) / (int32_t)len,
        .y = (dy * (1 << W2V_SHIFT)) / (int32_t)len,
        .z = (dz * (1 << W2V_SHIFT)) / (int32_t)len,
    };
}

static void M_SetConstantLight(const RGB_F ambient)
{
    const RGB_F colors[3] = {};
    const XYZ_32 dirs_view[3] = {};
    Output_SetTR3Light(ambient, colors, dirs_view);
}

static void M_CalculateLightSmoothed(
    const ITEM *const item, const XYZ_32 pos, const ROOM *const room)
{
    const LIGHT *sun_light = nullptr;
    bool has_sun = false;

    const LIGHT *brightest_light = nullptr;
    int32_t brightest = -1;
    XYZ_32 bulb_delta = {};

    int32_t ambience = ((SHADE_MAX - room->ambient) >> M_A_SHIFT) + 1;

    for (int32_t i = 0; i < room->num_lights; i++) {
        const LIGHT *const light = &room->lights[i];
        if (Output_Lights_IsSunLight(light)) {
            has_sun = true;
            sun_light = light;
            continue;
        }

        const int32_t falloff = Output_Lights_GetLightOuterRadius(light);
        if (falloff <= 0) {
            continue;
        }

        const int32_t dx = (light->pos.x - pos.x) >> 2;
        const int32_t dy = (light->pos.y - pos.y) >> 2;
        const int32_t dz = (light->pos.z - pos.z) >> 2;
        const uint32_t distance =
            Math_Sqrt((uint32_t)(SQUARE(dx) + SQUARE(dy) + SQUARE(dz)));

        if ((int32_t)distance > falloff) {
            continue;
        }

        const int32_t intensity = Output_Lights_GetLightIntensity(light);
        int32_t shade = intensity - (intensity * (int32_t)distance) / falloff;
        CLAMPL(shade, 0);
        ambience += shade >> 7;

        if (shade > brightest) {
            brightest = shade;
            brightest_light = light;
            bulb_delta = (XYZ_32) {
                .x = light->pos.x - pos.x,
                .y = light->pos.y - pos.y,
                .z = light->pos.z - pos.z,
            };
        }
    }

    const LIGHT *brightest_dynamic = nullptr;
    int32_t brightest_dyn_shade = -1;
    XYZ_32 dyn_delta = {};

    VECTOR *const dynamic_lights = Output_GetDynamicLights();
    for (int32_t i = 0; i < dynamic_lights->count; i++) {
        const OUTPUT_DYNAMIC_LIGHT *const entry = Vector_Get(dynamic_lights, i);
        const LIGHT *const light = &entry->light;
        const int32_t falloff_half =
            Output_Lights_GetLegacyData(light)->falloff.value_1 >> 1;
        if (falloff_half <= 0) {
            continue;
        }

        const int32_t dx = light->pos.x - pos.x;
        const int32_t dy = light->pos.y - pos.y;
        const int32_t dz = light->pos.z - pos.z;
        const int32_t max_dist = WALL_L * 8;
        if (ABS(dx) > max_dist || ABS(dy) > max_dist || ABS(dz) > max_dist) {
            continue;
        }

        const uint32_t distance =
            Math_Sqrt((uint32_t)(SQUARE(dx) + SQUARE(dy) + SQUARE(dz)));

        if ((int32_t)distance > falloff_half) {
            continue;
        }

        int32_t shade =
            SHADE_MAX - (SHADE_MAX * (int32_t)distance) / falloff_half;
        CLAMPL(shade, 0);
        ambience += shade >> 8;

        if (shade > brightest_dyn_shade) {
            brightest_dyn_shade = shade;
            brightest_dynamic = light;
            dyn_delta = (XYZ_32) {
                .x = light->pos.x - pos.x,
                .y = light->pos.y - pos.y,
                .z = light->pos.z - pos.z,
            };
        }
    }

    CLAMP(ambience, 0, 255);

    const uint8_t ambient_target = (uint8_t)ambience;

    M_ITEM_LIGHT dummy = {};
    M_ITEM_LIGHT *il = &dummy;
    bool enable_smoothing = false;
    if (item != nullptr) {
        const int16_t item_num = Item_GetIndex(item);
        if (item_num >= 0 && item_num < MAX_ITEMS) {
            il = &m_ItemLights[item_num];
            enable_smoothing = true;
        }
    }

    // Ambient (smoothed)
    if (enable_smoothing && il->flags.has_ambient) {
        il->ambient = M_LerpU8Shift(il->ambient, ambient_target, 3);
    } else {
        il->ambient = ambient_target;
        il->flags.has_ambient = true;
    }

    // Sun (smoothed)
    bool want_sun = false;
    XYZ_32 sun_dir_world_target = {};
    RGB_888 sun_target = {};

    int32_t ambient_base = (SHADE_MAX - room->ambient) >> M_A_SHIFT;
    CLAMP(ambient_base, 0, 255);

    if (has_sun && sun_light != nullptr) {
        want_sun = true;
        sun_dir_world_target = Output_Lights_GetLightDirWorld(sun_light);
        sun_target = sun_light->color;
    } else if (enable_smoothing && il->flags.has_sun) {
        want_sun = true;
        sun_dir_world_target = il->sun_dir_world;
        sun_target = (RGB_888) { ambient_base, ambient_base, ambient_base };
    }

    if (want_sun) {
        if (enable_smoothing && il->flags.has_sun) {
            il->sun_dir_world =
                M_LerpXYZShift(il->sun_dir_world, sun_dir_world_target, 3);
            il->sun_color = M_LerpRGBShift(il->sun_color, sun_target, 3);
        } else {
            il->sun_dir_world = sun_dir_world_target;
            il->sun_color = sun_target;
            il->flags.has_sun = true;
        }
    }

    // Bulb (smoothed)
    bool want_bulb = false;
    XYZ_32 bulb_dir_world_target = {};
    RGB_888 bulb_target = {};

    if (brightest_light != nullptr && brightest > 0) {
        want_bulb = true;
        bulb_dir_world_target = M_NormalizeDeltaWorld(bulb_delta);

        int32_t r8 = (brightest * (int32_t)brightest_light->color.r) >> 13;
        int32_t g8 = (brightest * (int32_t)brightest_light->color.g) >> 13;
        int32_t b8 = (brightest * (int32_t)brightest_light->color.b) >> 13;
        CLAMP(r8, 0, 255);
        CLAMP(g8, 0, 255);
        CLAMP(b8, 0, 255);
        bulb_target = (RGB_888) { r8, g8, b8 };
    } else if (enable_smoothing && il->flags.has_bulb) {
        want_bulb = true;
        bulb_dir_world_target = il->bulb_dir_world;
        bulb_target = (RGB_888) { ambient_base, ambient_base, ambient_base };
    }

    if (want_bulb) {
        if (enable_smoothing && il->flags.has_bulb) {
            il->bulb_dir_world =
                M_LerpXYZShift(il->bulb_dir_world, bulb_dir_world_target, 3);
            il->bulb_color = M_LerpRGBShift(il->bulb_color, bulb_target, 3);
        } else {
            il->bulb_dir_world = bulb_dir_world_target;
            il->bulb_color = bulb_target;
            il->flags.has_bulb = true;
        }
    }

    // Dynamic (smoothed while active, drops instantly when not present)
    bool want_dynamic = false;
    XYZ_32 dynamic_dir_world_target = {};
    RGB_888 dynamic_target = {};

    if (brightest_dynamic != nullptr && brightest_dyn_shade > 0) {
        want_dynamic = true;
        dynamic_dir_world_target = M_NormalizeDeltaWorld(dyn_delta);

        int32_t r8 =
            (brightest_dyn_shade * (int32_t)brightest_dynamic->color.r) >> 13;
        int32_t g8 =
            (brightest_dyn_shade * (int32_t)brightest_dynamic->color.g) >> 13;
        int32_t b8 =
            (brightest_dyn_shade * (int32_t)brightest_dynamic->color.b) >> 13;
        CLAMP(r8, 0, 255);
        CLAMP(g8, 0, 255);
        CLAMP(b8, 0, 255);
        dynamic_target = (RGB_888) { r8, g8, b8 };
    }

    if (want_dynamic) {
        if (enable_smoothing && il->flags.has_dynamic) {
            il->dynamic_dir_world = M_LerpXYZShift(
                il->dynamic_dir_world, dynamic_dir_world_target, 1);
            il->dynamic_color =
                M_LerpRGBShift(il->dynamic_color, dynamic_target, 1);
        } else {
            il->dynamic_dir_world = dynamic_dir_world_target;
            il->dynamic_color = dynamic_target;
            il->flags.has_dynamic = true;
        }
    }

    const RGB_F ambient = {
        .r = il->ambient / 255.0f,
        .g = il->ambient / 255.0f,
        .b = il->ambient / 255.0f,
    };

    RGB_F colors[3] = {};
    XYZ_32 dirs_view[3] = {};

    if (want_sun && il->flags.has_sun) {
        dirs_view[0] = Output_Lights_VectorViewFromWorld(il->sun_dir_world);
        colors[0] = (RGB_F) {
            .r = il->sun_color.r / 255.0f,
            .g = il->sun_color.g / 255.0f,
            .b = il->sun_color.b / 255.0f,
        };
    }

    if (want_bulb && il->flags.has_bulb) {
        dirs_view[1] = Output_Lights_VectorViewFromWorld(il->bulb_dir_world);
        colors[1] = (RGB_F) {
            .r = il->bulb_color.r / 255.0f,
            .g = il->bulb_color.g / 255.0f,
            .b = il->bulb_color.b / 255.0f,
        };
    }

    if (want_dynamic && il->flags.has_dynamic) {
        dirs_view[2] = Output_Lights_VectorViewFromWorld(il->dynamic_dir_world);
        colors[2] = (RGB_F) {
            .r = il->dynamic_color.r / 255.0f,
            .g = il->dynamic_color.g / 255.0f,
            .b = il->dynamic_color.b / 255.0f,
        };
    }

    Output_SetTR3Light(ambient, colors, dirs_view);

    // Keep legacy scalar shade meaningful for sprite/effect code paths.
    Output_SetLightDivider(0);
    Output_SetLightAdder(
        Output_Lights_ShadeFromMul((ambient.r + ambient.g + ambient.b) / 3.0f));
}

static void M_CalculateLight(const XYZ_32 pos, const int16_t room_num)
{
    M_CalculateLightSmoothed(nullptr, pos, Room_Get(room_num));
}

static void M_CalculateObjectLightingAt(
    const ITEM *const item, const GAME_VECTOR sample_pos)
{
    int16_t room_num = sample_pos.room_num;
    Room_GetSector(sample_pos.pos, &room_num);
    M_CalculateLightSmoothed(item, sample_pos.pos, Room_Get(room_num));
}

static void M_CalculateStaticLight(const int16_t adder)
{
    Output_Lights_SetScalarStaticLight(adder);

    float mul = 2.0f - (adder / (float)SHADE_NEUTRAL);
    CLAMP(mul, 0.0f, 1.0f);
    M_SetConstantLight((RGB_F) { mul, mul, mul });
}

static void M_CalculateStaticLightRGB15(const int16_t rgb15)
{
    M_SetConstantLight(M_RGB15ToRGBF(rgb15));
}

static void M_CalculateStaticLightRGB_F(const RGB_F rgb)
{
    M_SetConstantLight(rgb);
}

static void M_CalculateStaticMeshLight(
    const XYZ_32 pos, const SHADE shade, const ROOM *const room)
{
    const RGB_F base = M_RGB15ToRGBF(shade.value_1 & 0x7FFF);
    int32_t r = (int32_t)(base.r * 255.0f);
    int32_t g = (int32_t)(base.g * 255.0f);
    int32_t b = (int32_t)(base.b * 255.0f);

    VECTOR *const dynamic_lights = Output_GetDynamicLights();
    for (int32_t i = 0; i < dynamic_lights->count; i++) {
        const OUTPUT_DYNAMIC_LIGHT *const entry = Vector_Get(dynamic_lights, i);
        const LIGHT *const light = &entry->light;
        const int32_t falloff_half =
            Output_Lights_GetLegacyData(light)->falloff.value_1 >> 1;
        if (falloff_half <= 0) {
            continue;
        }
        const XYZ_32 delta = {
            .x = pos.x - light->pos.x,
            .y = pos.y - light->pos.y,
            .z = pos.z - light->pos.z,
        };
        const uint32_t distance = XYZ_32_GetLength(delta);
        if ((int32_t)distance > falloff_half) {
            continue;
        }

        int32_t fall =
            SHADE_MAX - (SHADE_MAX * (int32_t)distance) / falloff_half;
        CLAMPL(fall, 0);
        r += (fall * (int32_t)light->color.r) >> 13;
        g += (fall * (int32_t)light->color.g) >> 13;
        b += (fall * (int32_t)light->color.b) >> 13;
    }

    CLAMP(r, 0, 255);
    CLAMP(g, 0, 255);
    CLAMP(b, 0, 255);
    const RGB_F ambient = { r / 255.0f, g / 255.0f, b / 255.0f };
    M_SetConstantLight(ambient);

    Output_SetLightDivider(0);
    Output_SetLightAdder(
        Output_Lights_ShadeFromMul((ambient.r + ambient.g + ambient.b) / 3.0f));
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

    OUTPUT_UNIFORM_LS_TR3 ls = {};
    ls.ambient[0] = info->tr3_ambient.r;
    ls.ambient[1] = info->tr3_ambient.g;
    ls.ambient[2] = info->tr3_ambient.b;
    ls.ambient[3] = 0.0f;
    for (int32_t i = 0; i < 3; i++) {
        float x = (float)info->tr3_light_dir_view[i].x;
        float y = (float)info->tr3_light_dir_view[i].y;
        float z = (float)info->tr3_light_dir_view[i].z;
        const float len2 = x * x + y * y + z * z;
        if (len2 > 0.0f) {
            const float inv_len = 1.0f / sqrtf(len2);
            x *= inv_len;
            y *= inv_len;
            z *= inv_len;
        }
        ls.dir_view[i][0] = x;
        ls.dir_view[i][1] = y;
        ls.dir_view[i][2] = z;
        ls.dir_view[i][3] = 0.0f;
        ls.color[i][0] = info->tr3_light_color[i].r;
        ls.color[i][1] = info->tr3_light_color[i].g;
        ls.color[i][2] = info->tr3_light_color[i].b;
        ls.color[i][3] = 0.0f;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->ls);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(ls), &ls);
    TRX_GL_CheckError();
}

static void M_UploadOwnLight(
    const OUTPUT_UNIFORMS *const uniforms, const OUTPUT_LIGHT_INFO *const info)
{
    OUTPUT_LS_CACHE *const cache = Output_Lights_GetLSCache();
    if (cache->mode == LS_MODE_OWN
        && cache->last_own_tr3_ambient.r == info->tr3_ambient.r
        && cache->last_own_tr3_ambient.g == info->tr3_ambient.g
        && cache->last_own_tr3_ambient.b == info->tr3_ambient.b) {
        return;
    }

    const float ambient[4] = {
        info->tr3_ambient.r,
        info->tr3_ambient.g,
        info->tr3_ambient.b,
        0.0f,
    };
    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->ls);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER,
        offsetof(OUTPUT_UNIFORM_LS_TR3, ambient), sizeof(ambient), ambient);
    cache->last_own_tr3_ambient = info->tr3_ambient;
    cache->mode = LS_MODE_OWN;
}

static void M_Init(void)
{
    memset(m_ItemLights, 0, sizeof(m_ItemLights));
}

static void M_ObserveLevelLoad(void)
{
    memset(m_ItemLights, 0, sizeof(m_ItemLights));
}

void Output_Lights_TR3_AddDynamicLight(
    const XYZ_32 pos, const int32_t intensity, const int32_t falloff)
{
    int32_t safe_intensity = intensity;
    int32_t safe_falloff = falloff;
    CLAMP(safe_intensity, 0, 30);
    CLAMP(safe_falloff, 0, 30);

    int32_t max_shade = 1 << safe_intensity;
    int32_t c = max_shade >> 4;
    CLAMPG(c, 255);

    int32_t radius = 1 << safe_falloff;
    int32_t falloff_param = radius >> 7;
    CLAMP(falloff_param, 1, 255);

    const OUTPUT_DYNAMIC_LIGHT light = {
        .light = {
            .pos = pos,
            .color = (RGB_888) { c, c, c },
            .layout = LIGHT_LAYOUT_LEGACY,
            .type = LIGHT_TYPE_POINT,
            .u.legacy =
                {
                    .falloff.value_1 = falloff_param
                        << OUTPUT_DYNAMIC_FALLOFF_SHIFT,
                },
        },
        .kind = OUTPUT_DYNAMIC_LIGHT_RGB,
    };
    Vector_Add(Output_GetDynamicLights(), &light);
}

const LIGHTING_MODEL g_LightingModelTR3 = {
    .init = M_Init,
    .observe_level_load = M_ObserveLevelLoad,
    .calculate_light = M_CalculateLight,
    .calculate_object_lighting_at = M_CalculateObjectLightingAt,
    .calculate_static_light = M_CalculateStaticLight,
    .calculate_static_light_rgb15 = M_CalculateStaticLightRGB15,
    .calculate_static_light_rgb_f = M_CalculateStaticLightRGB_F,
    .calculate_static_mesh_light = M_CalculateStaticMeshLight,
    .add_dynamic_light = Output_Lights_TR3_AddDynamicLight,
    .upload_cpu_light = M_UploadCPULight,
    .upload_own_light = M_UploadOwnLight,
    .shader_variant = 1,
};
