#include <trx/core/colors.h>
#include <trx/core/math/geom.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/camera/vars.h>
#include <trx/game/const.h>
#include <trx/game/matrix.h>
#include <trx/game/output.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/lights/fog_bulbs.h>
#include <trx/game/output/lights/priv.h>
#include <trx/game/random.h>
#include <trx/version.h>

#include <string.h>

#define M_MAX_ROOM_LIGHT_UNIT (0x2000 / (OUTPUT_LIGHT_CYCLE / 2))

typedef struct {
    int32_t table[OUTPUT_LIGHT_CYCLE];
} M_ROOM_LIGHT_TABLE;

typedef struct {
    XYZ_32 pos;
    RGB_888 color;
    int32_t intensity;
    int32_t falloff;
    int32_t cam_dist;
    bool is_rgb;
} M_PENDING_LIGHT;

typedef struct {
    XYZ_32 pos;
    RGB_888 color;
    int32_t radius;
    int32_t density;
    int32_t cam_dist;
} M_PENDING_FOG;

static bool m_IsSunsetEnabled = false;
static int32_t m_RoomLightShades[RLM_NUMBER_OF] = {};
static M_ROOM_LIGHT_TABLE m_RoomLightTables[OUTPUT_LIGHT_CYCLE] = {};
static VECTOR *m_DynamicLights = nullptr;
static OUTPUT_LS_CACHE m_LSCache = {};

static struct {
    int32_t count;
    M_PENDING_LIGHT list[OUTPUT_MAX_PENDING_LIGHTS];
} m_PendingLights = {};

static struct {
    int32_t count;
    M_PENDING_FOG list[OUTPUT_MAX_FOG_BULBS];
} m_PendingFog = {};

static const LIGHTING_MODEL *const m_Models[] = {
    &g_LightingModelTR12,
    &g_LightingModelTR3,
    &g_LightingModelTR4,
};

static void M_AddDynamicLight(
    const XYZ_32 pos, const int32_t intensity, const int32_t falloff)
{
    Output_Lights_GetModel()->add_dynamic_light(pos, intensity, falloff);
}

static void M_AddDynamicLightRGB(
    const XYZ_32 pos, const int32_t falloff, const RGB_888 color)
{
    const LIGHTING_MODEL *const model = Output_Lights_GetModel();
    if (model->add_dynamic_light_rgb != nullptr) {
        model->add_dynamic_light_rgb(pos, falloff, color);
        return;
    }

    int32_t safe_falloff = falloff;
    CLAMP(safe_falloff, 0, OUTPUT_DYNAMIC_FALLOFF_MAX);

    const OUTPUT_DYNAMIC_LIGHT light = {
        .light = {
            .pos = pos,
            .color = color,
            .layout = LIGHT_LAYOUT_LEGACY,
            .type = LIGHT_TYPE_POINT,
            .u.legacy =
                {
                    .falloff.value_1 = safe_falloff
                        << OUTPUT_DYNAMIC_FALLOFF_SHIFT,
                },
        },
        .kind = OUTPUT_DYNAMIC_LIGHT_RGB,
    };
    Vector_Add(m_DynamicLights, &light);
}

static void M_AddPendingLight(M_PENDING_LIGHT light)
{
    light.cam_dist = XYZ_32_GetDistance(light.pos, g_Camera.pos.pos);

    if (m_PendingLights.count < OUTPUT_MAX_PENDING_LIGHTS) {
        m_PendingLights.list[m_PendingLights.count++] = light;
        return;
    }

    int32_t furthest_idx = -1;
    int32_t furthest_dist = light.cam_dist;
    for (int32_t i = 0; i < m_PendingLights.count; i++) {
        if (m_PendingLights.list[i].cam_dist > furthest_dist) {
            furthest_dist = m_PendingLights.list[i].cam_dist;
            furthest_idx = i;
        }
    }
    if (furthest_idx >= 0) {
        m_PendingLights.list[furthest_idx] = light;
    }
}

const LIGHTING_MODEL *Output_Lights_GetModel(void)
{
    return m_Models[g_TRVersion <= 2 ? 0 : (g_TRVersion == 3 ? 1 : 2)];
}

OUTPUT_LS_CACHE *Output_Lights_GetLSCache(void)
{
    return &m_LSCache;
}

size_t Output_Lights_GetLSBufferSize(void)
{
    return MAX(
        MAX(sizeof(OUTPUT_UNIFORM_LS_TR12), sizeof(OUTPUT_UNIFORM_LS_TR3)),
        sizeof(OUTPUT_UNIFORM_LS_TR4));
}

void Output_Lights_ResetUploadCache(void)
{
    memset(&m_LSCache, 0, sizeof(m_LSCache));
    Output_FogBulbs_ResetUploadCache();
}

const LIGHT_LEGACY_DATA *Output_Lights_GetLegacyData(const LIGHT *const light)
{
    return &light->u.legacy;
}

bool Output_Lights_IsSunLight(const LIGHT *const light)
{
    return light->type == LIGHT_TYPE_SUN;
}

int32_t Output_Lights_GetLightIntensity(const LIGHT *const light)
{
    if (light->layout == LIGHT_LAYOUT_TR4) {
        return light->u.tr4.intensity;
    }
    return Output_Lights_GetLegacyData(light)->shade.value_1;
}

int32_t Output_Lights_GetLightOuterRadius(const LIGHT *const light)
{
    if (light->layout == LIGHT_LAYOUT_TR4) {
        return (int32_t)light->u.tr4.outer_radius;
    }
    return Output_Lights_GetLegacyData(light)->falloff.value_1;
}

XYZ_32 Output_Lights_GetLightDirWorld(const LIGHT *const light)
{
    if (light->layout == LIGHT_LAYOUT_TR4) {
        return (XYZ_32) {
            .x = (int32_t)(light->u.tr4.dir.x * (1 << W2V_SHIFT)),
            .y = (int32_t)(light->u.tr4.dir.y * (1 << W2V_SHIFT)),
            .z = (int32_t)(light->u.tr4.dir.z * (1 << W2V_SHIFT)),
        };
    }
    const XYZ_16 dir = Output_Lights_GetLegacyData(light)->dir;
    return (XYZ_32) {
        .x = dir.x,
        .y = dir.y,
        .z = dir.z,
    };
}

XYZ_32 Output_Lights_VectorViewFromWorld(const XYZ_32 v_world)
{
    const MATRIX *const m = &g_ViewMatrix;
    return (XYZ_32) {
        .x = (m->_00 * v_world.x + m->_01 * v_world.y + m->_02 * v_world.z)
            >> W2V_SHIFT,
        .y = (m->_10 * v_world.x + m->_11 * v_world.y + m->_12 * v_world.z)
            >> W2V_SHIFT,
        .z = (m->_20 * v_world.x + m->_21 * v_world.y + m->_22 * v_world.z)
            >> W2V_SHIFT,
    };
}

int16_t Output_Lights_ShadeFromMul(const float mul)
{
    float shade_f = (2.0f - mul) * (float)SHADE_NEUTRAL;
    CLAMP(shade_f, 0.0f, SHADE_MAX);
    return (int16_t)shade_f;
}

void Output_Lights_SetScalarStaticLight(const int16_t adder)
{
    // TODO: use m_LsAdder
    int32_t global_adder = adder - SHADE_NEUTRAL;
    CLAMPG(global_adder, SHADE_MAX);
    Output_SetLightAdder(global_adder);
}

void Output_CalculateLight(const XYZ_32 pos, const int16_t room_num)
{
    Output_Lights_GetModel()->calculate_light(pos, room_num);
}

void Output_CalculateStaticLight(const int16_t adder)
{
    Output_Lights_GetModel()->calculate_static_light(adder);
}

void Output_CalculateStaticLightRGB15(const int16_t rgb15)
{
    const LIGHTING_MODEL *const model = Output_Lights_GetModel();
    if (model->calculate_static_light_rgb15 != nullptr) {
        model->calculate_static_light_rgb15(rgb15);
    }
}

void Output_CalculateStaticLightRGB_F(const RGB_F rgb)
{
    const LIGHTING_MODEL *const model = Output_Lights_GetModel();
    if (model->calculate_static_light_rgb_f != nullptr) {
        model->calculate_static_light_rgb_f(rgb);
    }
}

void Output_CalculateStaticMeshLight(
    const XYZ_32 pos, const SHADE shade, const ROOM *const room)
{
    Output_Lights_GetModel()->calculate_static_mesh_light(pos, shade, room);
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
    Matrix_TranslateSet32((XYZ_32) {});
    Matrix_Rot16(item->rot);
    Matrix_TranslateRel32((XYZ_32) {
        .x = (bounds->min.x + bounds->max.x) / 2,
        .y = (bounds->max.y + bounds->min.y) / 2,
        .z = (bounds->max.z + bounds->min.z) / 2,
    });
    const GAME_VECTOR sample_pos = {
        .room_num = item->room_num,
        .pos = {
            .x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT),
            .y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT),
            .z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT),
        },
    };
    Matrix_Pop();

    Output_CalculateObjectLightingAt(item, sample_pos);
}

void Output_CalculateObjectLightingAt(
    const ITEM *const item, const GAME_VECTOR sample_pos)
{
    Output_Lights_GetModel()->calculate_object_lighting_at(item, sample_pos);
}

void Output_Lights_Init(void)
{
    if (m_DynamicLights == nullptr) {
        m_DynamicLights = Vector_Create(sizeof(OUTPUT_DYNAMIC_LIGHT));
    }

    for (int32_t i = 0; i < OUTPUT_LIGHT_CYCLE; i++) {
        for (int32_t j = 0; j < OUTPUT_LIGHT_CYCLE; j++) {
            m_RoomLightTables[i].table[j] = (j - (OUTPUT_LIGHT_CYCLE / 2)) * i
                * M_MAX_ROOM_LIGHT_UNIT / (OUTPUT_LIGHT_CYCLE - 1);
        }
    }

    // The game version may change between levels, so initialize every model
    // rather than just the current one.
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Models); i++) {
        if (i > 0 && m_Models[i] == m_Models[i - 1]) {
            continue;
        }
        if (m_Models[i]->init != nullptr) {
            m_Models[i]->init();
        }
    }
}

void Output_Lights_Shutdown(void)
{
    if (m_DynamicLights != nullptr) {
        Vector_Free(m_DynamicLights);
        m_DynamicLights = nullptr;
    }

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Models); i++) {
        if (i > 0 && m_Models[i] == m_Models[i - 1]) {
            continue;
        }
        if (m_Models[i]->shutdown != nullptr) {
            m_Models[i]->shutdown();
        }
    }
}

void Output_Lights_ObserveLevelLoad(void)
{
    // Fog bulbs are shared by every game, so they are cleared here rather than
    // by the model that happens to fill them. This runs before the models so
    // that the bulbs a TR4 level carries survive the clear.
    Output_FogBulbs_Reset();

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Models); i++) {
        if (i > 0 && m_Models[i] == m_Models[i - 1]) {
            continue;
        }
        if (m_Models[i]->observe_level_load != nullptr) {
            m_Models[i]->observe_level_load();
        }
    }
}

void Output_Lights_BeginScene(void)
{
    const LIGHTING_MODEL *const model = Output_Lights_GetModel();
    if (model->begin_scene != nullptr) {
        model->begin_scene();
    }
}

void Output_Lights_PrepareScene(void)
{
    const LIGHTING_MODEL *const model = Output_Lights_GetModel();
    if (model->prepare_scene != nullptr) {
        model->prepare_scene();
    }
}

void Output_Lights_FillInstanceLight(
    OUTPUT_LIGHT_INFO *const info, const MATRIX *const wmatrix)
{
    const LIGHTING_MODEL *const model = Output_Lights_GetModel();
    if (model->fill_instance_light != nullptr) {
        model->fill_instance_light(info, wmatrix);
    }
}

void Output_Lights_UploadCPULight(const OUTPUT_LIGHT_INFO *const info)
{
    Output_Lights_GetModel()->upload_cpu_light(Output_GetUniforms(), info);
}

void Output_Lights_UploadOwnLight(const OUTPUT_LIGHT_INFO *const info)
{
    Output_Lights_GetModel()->upload_own_light(Output_GetUniforms(), info);
}

void Output_ResetDynamicLights(void)
{
    Vector_Clear(m_DynamicLights);
    Output_FogBulbs_ResetFrame();
}

void Output_AddDynamicLight(
    const XYZ_32 pos, const int32_t intensity, const int32_t falloff)
{
    M_AddPendingLight((M_PENDING_LIGHT) {
        .pos = pos,
        .intensity = intensity,
        .falloff = falloff,
    });
}

void Output_AddDynamicLightRGB(
    const XYZ_32 pos, const int32_t falloff, const RGB_888 color)
{
    M_AddPendingLight((M_PENDING_LIGHT) {
        .pos = pos,
        .color = color,
        .falloff = falloff,
        .is_rgb = true,
    });
}

void Output_AddFogBulb(
    const XYZ_32 pos, const int32_t radius, const int32_t density,
    const RGB_888 color)
{
    const M_PENDING_FOG fog = {
        .pos = pos,
        .color = color,
        .radius = radius,
        .density = density,
        .cam_dist = XYZ_32_GetDistance(pos, g_Camera.pos.pos),
    };

    if (m_PendingFog.count < OUTPUT_MAX_FOG_BULBS) {
        m_PendingFog.list[m_PendingFog.count++] = fog;
        return;
    }

    // The buffer keeps the nearest bulbs rather than the first ones asked for,
    // as in M_AddPendingLight.
    int32_t furthest_idx = -1;
    int32_t furthest_dist = fog.cam_dist;
    for (int32_t i = 0; i < m_PendingFog.count; i++) {
        if (m_PendingFog.list[i].cam_dist > furthest_dist) {
            furthest_dist = m_PendingFog.list[i].cam_dist;
            furthest_idx = i;
        }
    }
    if (furthest_idx >= 0) {
        m_PendingFog.list[furthest_idx] = fog;
    }
}

void Output_DropPendingLights(void)
{
    m_PendingLights.count = 0;
    m_PendingFog.count = 0;
}

void Output_FlushPendingLights(void)
{
    for (int32_t i = 0; i < m_PendingLights.count; i++) {
        const M_PENDING_LIGHT *const queued = &m_PendingLights.list[i];
        if (queued->is_rgb) {
            M_AddDynamicLightRGB(queued->pos, queued->falloff, queued->color);
        } else {
            M_AddDynamicLight(queued->pos, queued->intensity, queued->falloff);
        }
    }
    m_PendingLights.count = 0;

    for (int32_t i = 0; i < m_PendingFog.count; i++) {
        const M_PENDING_FOG *const queued = &m_PendingFog.list[i];
        Output_FogBulbs_AddFrame(
            queued->pos, queued->radius, queued->density, queued->color);
    }
    m_PendingFog.count = 0;
}

void Output_TriggerFXFogBulb(
    const XYZ_32 pos, const int32_t fx_rad, const int32_t density,
    const RGB_888 color)
{
    // The OG triggers these for underwater flares and shimmer, which no other
    // game has.
    if (g_TRVersion != 4) {
        return;
    }
    Output_FogBulbs_AddTimed(pos, fx_rad, density, color);
}

VECTOR *Output_GetDynamicLights(void)
{
    return m_DynamicLights;
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
    const int32_t time = ((int32_t)Output_GetTimeInGame()) % OUTPUT_LIGHT_CYCLE;
    if (g_TRVersion >= 2) {
        m_RoomLightShades[RLM_FLICKER] = Random_GetDraw() % OUTPUT_LIGHT_CYCLE;
        m_RoomLightShades[RLM_GLOW] = (OUTPUT_LIGHT_CYCLE - 1)
                * (Math_Sin((time * DEG_360) / OUTPUT_LIGHT_CYCLE) + 0x4000)
            >> 15;

        if (m_IsSunsetEnabled) {
            int32_t sunset_timer = Output_GetTimeInGame();
            CLAMPG(sunset_timer, Output_GetSunsetDuration());
            m_RoomLightShades[RLM_SUNSET] = sunset_timer
                * (OUTPUT_LIGHT_CYCLE - 1) / Output_GetSunsetDuration();
        }
    }

    const LIGHTING_MODEL *const model = Output_Lights_GetModel();
    if (model->animate != nullptr) {
        model->animate(num_frames);
    }
}
