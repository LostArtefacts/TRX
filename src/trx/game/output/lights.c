#include <trx/game/output/lights.h>

#include <trx/colors.h>
#include <trx/game/matrix.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/utils.h>
#include <trx/vector.h>
#include <trx/version.h>

#define M_LIGHT_CYCLE 32
#define M_MAX_ROOM_LIGHT_UNIT (0x2000 / (M_LIGHT_CYCLE / 2))
#define M_TR3_A_SHIFT 5
#define M_TR3_DYNAMIC_FALLOFF_SHIFT 8

typedef struct {
    XYZ_32 pos;
    int32_t shade;
} M_COMMON_LIGHT;

typedef struct {
    int32_t table[M_LIGHT_CYCLE];
} M_ROOM_LIGHT_TABLE;

static bool m_IsSunsetEnabled = false;
static int32_t m_RoomLightShades[RLM_NUMBER_OF] = {};
static M_ROOM_LIGHT_TABLE m_RoomLightTables[M_LIGHT_CYCLE] = {};
static VECTOR *m_DynamicLights = nullptr;

static RGB_F M_TR3_RGB15ToRGBF(const int16_t rgb15)
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

static int16_t M_TR3_ShadeFromMul(const float mul)
{
    float shade_f = (2.0f - mul) * (float)SHADE_NEUTRAL;
    CLAMP(shade_f, 0.0f, SHADE_MAX);
    return (int16_t)shade_f;
}

static XYZ_32 M_VectorViewFromAngles(const int16_t pitch, const int16_t yaw)
{
    const int32_t cp = Math_Cos(pitch);
    const int32_t sp = Math_Sin(pitch);
    const int32_t cy = Math_Cos(yaw);
    const int32_t sy = Math_Sin(yaw);
    const int32_t x = TRIGMULT2(cp, sy);
    const int32_t y = -sp;
    const int32_t z = TRIGMULT2(cp, cy);
    const MATRIX *const m = &g_ViewMatrix;
    return (XYZ_32) {
        .x = (m->_00 * x + m->_01 * y + m->_02 * z) >> W2V_SHIFT,
        .y = (m->_10 * x + m->_11 * y + m->_12 * z) >> W2V_SHIFT,
        .z = (m->_20 * x + m->_21 * y + m->_22 * z) >> W2V_SHIFT,
    };
}

static XYZ_32 M_VectorViewFromDelta(const XYZ_32 delta)
{
    int16_t angles[2];
    Math_GetVectorAngles(delta.x, delta.y, delta.z, angles);
    return M_VectorViewFromAngles(angles[1], angles[0]);
}

static void M_TR3_SetConstantLight(const RGB_F ambient)
{
    const RGB_F colors[3] = {};
    const XYZ_32 dirs_view[3] = {};
    Output_SetTR3Light(ambient, colors, dirs_view);
}

static void M_CalculateBrightestLight(
    const XYZ_32 pos, const ROOM *const room,
    M_COMMON_LIGHT *const brightest_light)
{
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
                falloff_1 * light->shade.value_1 / MAX(1, falloff_1 + dist);
            const int32_t shade_2 =
                falloff_2 * light->shade.value_2 / MAX(1, falloff_2 + dist);
            const int32_t shade = shade_1
                + (shade_2 - shade_1) * light_shade / (M_LIGHT_CYCLE - 1);

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
    const XYZ_32 pos, M_COMMON_LIGHT *const brightest_light)
{
    int32_t adder = 0;
    for (int32_t i = 0; i < m_DynamicLights->count; i++) {
        const LIGHT *const light = Vector_Get(m_DynamicLights, i);
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

static void M_TR3_CalculateLight(const XYZ_32 pos, const ROOM *const room)
{
    const LIGHT *sun_light = nullptr;
    bool has_sun = false;

    const LIGHT *brightest_light = nullptr;
    int32_t brightest = -1;
    XYZ_32 bulb_delta = {};

    int32_t ambience = ((SHADE_MAX - room->ambient) >> M_TR3_A_SHIFT) + 1;

    for (int32_t i = 0; i < room->num_lights; i++) {
        const LIGHT *const light = &room->lights[i];

        if (light->type != 0u) {
            has_sun = true;
            sun_light = light;
            continue;
        }

        const int32_t falloff = light->falloff.value_1;
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

        const int32_t intensity = light->shade.value_1;
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

    for (int32_t i = 0; i < m_DynamicLights->count; i++) {
        const LIGHT *const light = Vector_Get(m_DynamicLights, i);
        const int32_t falloff_half = light->falloff.value_1 >> 1;
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

    const RGB_F ambient = {
        .r = ambience / 255.0f,
        .g = ambience / 255.0f,
        .b = ambience / 255.0f,
    };

    RGB_F colors[3] = {};
    XYZ_32 dirs_view[3] = {};

    if (has_sun && sun_light != nullptr) {
        const XYZ_32 sun_dir_world = {
            .x = sun_light->dir.x,
            .y = sun_light->dir.y,
            .z = sun_light->dir.z,
        };
        const MATRIX *const m = &g_ViewMatrix;
        dirs_view[0] = (XYZ_32) {
            .x = (m->_00 * sun_dir_world.x + m->_01 * sun_dir_world.y
                  + m->_02 * sun_dir_world.z)
                >> W2V_SHIFT,
            .y = (m->_10 * sun_dir_world.x + m->_11 * sun_dir_world.y
                  + m->_12 * sun_dir_world.z)
                >> W2V_SHIFT,
            .z = (m->_20 * sun_dir_world.x + m->_21 * sun_dir_world.y
                  + m->_22 * sun_dir_world.z)
                >> W2V_SHIFT,
        };
        colors[0] = (RGB_F) {
            .r = sun_light->color.r / 255.0f,
            .g = sun_light->color.g / 255.0f,
            .b = sun_light->color.b / 255.0f,
        };
    }

    if (brightest_light != nullptr && brightest > 0) {
        dirs_view[1] = M_VectorViewFromDelta(bulb_delta);
        int32_t r8 = (brightest * (int32_t)brightest_light->color.r) >> 13;
        int32_t g8 = (brightest * (int32_t)brightest_light->color.g) >> 13;
        int32_t b8 = (brightest * (int32_t)brightest_light->color.b) >> 13;
        CLAMP(r8, 0, 255);
        CLAMP(g8, 0, 255);
        CLAMP(b8, 0, 255);
        colors[1] = (RGB_F) {
            .r = r8 / 255.0f,
            .g = g8 / 255.0f,
            .b = b8 / 255.0f,
        };
    }

    if (brightest_dynamic != nullptr && brightest_dyn_shade > 0) {
        dirs_view[2] = M_VectorViewFromDelta(dyn_delta);
        int32_t r8 =
            (brightest_dyn_shade * (int32_t)brightest_dynamic->color.r) >> 13;
        int32_t g8 =
            (brightest_dyn_shade * (int32_t)brightest_dynamic->color.g) >> 13;
        int32_t b8 =
            (brightest_dyn_shade * (int32_t)brightest_dynamic->color.b) >> 13;
        CLAMP(r8, 0, 255);
        CLAMP(g8, 0, 255);
        CLAMP(b8, 0, 255);
        colors[2] = (RGB_F) {
            .r = r8 / 255.0f,
            .g = g8 / 255.0f,
            .b = b8 / 255.0f,
        };
    }

    Output_SetTR3Light(ambient, colors, dirs_view);

    // Keep legacy scalar shade meaningful for sprite/effect code paths.
    Output_SetLightDivider(0);
    Output_SetLightAdder(
        M_TR3_ShadeFromMul((ambient.r + ambient.g + ambient.b) / 3.0f));
}

void Output_CalculateLight(const XYZ_32 pos, const int16_t room_num)
{
    const ROOM *const room = Room_Get(room_num);

    if (g_TRVersion >= 3) {
        M_TR3_CalculateLight(pos, room);
        return;
    }

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

void Output_CalculateStaticLight(const int16_t adder)
{
    // TODO: use m_LsAdder
    int32_t global_adder = adder - SHADE_NEUTRAL;
    CLAMPG(global_adder, SHADE_MAX);
    Output_SetLightAdder(global_adder);

    if (g_TRVersion >= 3) {
        float mul = 2.0f - (adder / (float)SHADE_NEUTRAL);
        CLAMP(mul, 0.0f, 1.0f);
        M_TR3_SetConstantLight((RGB_F) { mul, mul, mul });
    }
}

void Output_CalculateStaticLightRGB15(const int16_t rgb15)
{
    if (g_TRVersion < 3) {
        return;
    }
    M_TR3_SetConstantLight(M_TR3_RGB15ToRGBF(rgb15));
}

void Output_CalculateStaticLightRGB_F(const RGB_F rgb)
{
    if (g_TRVersion < 3) {
        return;
    }
    M_TR3_SetConstantLight(rgb);
}

void Output_CalculateStaticMeshLight(
    const XYZ_32 pos, const SHADE shade, const ROOM *const room)
{
    if (g_TRVersion >= 3) {
        const RGB_F base = M_TR3_RGB15ToRGBF(shade.value_1 & 0x7FFF);
        int32_t r = (int32_t)(base.r * 255.0f);
        int32_t g = (int32_t)(base.g * 255.0f);
        int32_t b = (int32_t)(base.b * 255.0f);

        for (int32_t i = 0; i < m_DynamicLights->count; i++) {
            const LIGHT *const light = Vector_Get(m_DynamicLights, i);
            const int32_t falloff_half = light->falloff.value_1 >> 1;
            if (falloff_half <= 0) {
                continue;
            }
            const int32_t dx = pos.x - light->pos.x;
            const int32_t dy = pos.y - light->pos.y;
            const int32_t dz = pos.z - light->pos.z;

            const uint32_t distance =
                Math_Sqrt((uint32_t)(SQUARE(dx) + SQUARE(dy) + SQUARE(dz)));
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
        M_TR3_SetConstantLight(ambient);

        Output_SetLightDivider(0);
        Output_SetLightAdder(
            M_TR3_ShadeFromMul((ambient.r + ambient.g + ambient.b) / 3.0f));
        return;
    }

    int32_t adder = shade.value_1;
    if (room->light_mode != RLM_NORMAL) {
        const int32_t room_shade = Output_GetRoomLightShade(room->light_mode);
        adder +=
            (shade.value_2 - shade.value_1) * room_shade / (M_LIGHT_CYCLE - 1);
    }

    for (int32_t i = 0; i < m_DynamicLights->count; i++) {
        const LIGHT *const light = Vector_Get(m_DynamicLights, i);
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

void Output_InitLight(void)
{
    if (m_DynamicLights == nullptr) {
        m_DynamicLights = Vector_Create(sizeof(LIGHT));
    }

    for (int32_t i = 0; i < M_LIGHT_CYCLE; i++) {
        for (int32_t j = 0; j < M_LIGHT_CYCLE; j++) {
            m_RoomLightTables[i].table[j] = (j - (M_LIGHT_CYCLE / 2)) * i
                * M_MAX_ROOM_LIGHT_UNIT / (M_LIGHT_CYCLE - 1);
        }
    }
}

void Output_ShutdownLight(void)
{
    if (m_DynamicLights != nullptr) {
        Vector_Free(m_DynamicLights);
        m_DynamicLights = nullptr;
    }
}

void Output_ResetDynamicLights(void)
{
    Vector_Clear(m_DynamicLights);
}

VECTOR *Output_GetDynamicLights(void)
{
    return m_DynamicLights;
}

void Output_AddDynamicLight(
    const XYZ_32 pos, const int32_t intensity, const int32_t falloff)
{
    if (g_TRVersion >= 3) {
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

        const LIGHT light = {
            .pos = pos,
            .shade = {},
            .falloff.value_1 = falloff_param << M_TR3_DYNAMIC_FALLOFF_SHIFT,
            .color = (RGB_888) { (uint8_t)c, (uint8_t)c, (uint8_t)c },
            .type = 0,
            .dir = {},
        };
        Vector_Add(m_DynamicLights, &light);
    } else {
        const LIGHT light = {
            .pos = pos,
            .shade.value_1 = intensity,
            .falloff.value_1 = falloff,
            .color = (RGB_888) { 255, 255, 255 },
            .type = 0,
            .dir = {},
        };
        Vector_Add(m_DynamicLights, &light);
    }
}

void Output_AddDynamicLightRGB(
    const XYZ_32 pos, const int32_t falloff, const RGB_888 color)
{
    if (g_TRVersion < 3) {
        return;
    }

    int32_t safe_falloff = falloff;
    CLAMP(safe_falloff, 0, 255);

    const LIGHT light = {
        .pos = pos,
        .shade = {},
        .falloff.value_1 = safe_falloff << M_TR3_DYNAMIC_FALLOFF_SHIFT,
        .color = color,
        .type = 0,
        .dir = {},
    };
    Vector_Add(m_DynamicLights, &light);
}

int32_t Output_GetRoomLightShade(const ROOM_LIGHT_MODE mode)
{
    return m_RoomLightShades[mode];
}

int32_t Output_GetSunsetDuration(void)
{
    return 20 * 60 * LOGIC_FPS; // = 20 minutes / 36000 frames
}

void Output_SetSunsetEnabled(const bool enabled)
{
    m_IsSunsetEnabled = enabled;
}

int16_t Output_GetSkyShade(void)
{
    if (!m_IsSunsetEnabled) {
        return SHADE_NEUTRAL;
    }
    float sunset_progress =
        Output_GetTimeInGame() / (float)Output_GetSunsetDuration();
    CLAMP(sunset_progress, 0.0f, 1.0f);
    return SHADE_NEUTRAL + SHADE_SUNSET * sunset_progress;
}

void Output_AnimateLights(const int32_t num_frames)
{
    const int32_t time = ((int32_t)Output_GetTimeInGame()) % M_LIGHT_CYCLE;
    if (g_TRVersion >= 2) {
        m_RoomLightShades[RLM_FLICKER] = Random_GetDraw() % M_LIGHT_CYCLE;
        m_RoomLightShades[RLM_GLOW] = (M_LIGHT_CYCLE - 1)
                * (Math_Sin((time * DEG_360) / M_LIGHT_CYCLE) + 0x4000)
            >> 15;

        if (m_IsSunsetEnabled) {
            int32_t sunset_timer = Output_GetTimeInGame();
            CLAMPG(sunset_timer, Output_GetSunsetDuration());
            m_RoomLightShades[RLM_SUNSET] =
                sunset_timer * (M_LIGHT_CYCLE - 1) / Output_GetSunsetDuration();
        }
    }
}
