#define M_MAX_LIGHTS 32

#include <trx/game/output/uniforms.h>

#include <trx/config.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/output.h>
#include <trx/game/output/lights/priv.h>
#include <trx/game/output/utils.h>
#include <trx/game/rooms.h>
#include <trx/gl/utils.h>
#include <trx/version.h>

#include <math.h>
#include <string.h>

#define M_GLOBAL_MEMBERS                                                       \
    X_DECLARE_MEMBER(float, fog_color, [4])                                    \
    X_DECLARE_MEMBER(float, fog_distance, [2])                                 \
    X_DECLARE_MEMBER(float, viewport_size, [2])                                \
    X_DECLARE_MEMBER(float, time)                                              \
    X_DECLARE_MEMBER(float, time_in_game)                                      \
    X_DECLARE_MEMBER(float, brightness_multiplier)                             \
    X_DECLARE_MEMBER(float, ui_brightness_multiplier)                          \
    X_DECLARE_MEMBER(float, gamma)                                             \
    X_DECLARE_MEMBER(float, sunset_duration)                                   \
    X_DECLARE_MEMBER(float, min_shade)                                         \
    X_DECLARE_MEMBER(int, billboard_lock_mode)                                 \
    X_DECLARE_MEMBER(int, lighting_enabled)                                    \
    X_DECLARE_MEMBER(int, trapezoid_filter_enabled)                            \
    X_DECLARE_MEMBER(int, reflections_enabled)                                 \
    X_DECLARE_MEMBER(int, textures_enabled)                                    \
    X_DECLARE_MEMBER(int, tr_version)                                          \
    X_DECLARE_MEMBER(float, uv_rotate_offset)

#pragma pack(push, 4)
typedef struct {
#define X_DECLARE_MEMBER(a, b, ...) a b __VA_ARGS__;
    M_GLOBAL_MEMBERS
#undef X_DECLARE_MEMBER
} M_UNIFORM_GENERAL;

typedef struct {
    float mat_proj[4][4];
    float mat_view[4][4];
} M_UNIFORM_MATRICES;

typedef struct {
    float pos[4];
    float color[4];
    float shade;
    float falloff;
    float kind;
    float _pad;
} M_UNIFORM_LIGHT;

typedef struct {
    int num_lights;
    int room_light_mode;
    int _pad[2];
    M_UNIFORM_LIGHT lights[M_MAX_LIGHTS];
} M_UNIFORM_LIGHTS;

#pragma pack(pop)

typedef struct {
    M_UNIFORM_LIGHTS last_lights;
} M_PRIV;

static void M_FillLight(
    M_UNIFORM_LIGHT *const dst_light, const LIGHT *const src_light)
{
    dst_light->pos[0] = src_light->pos.x;
    dst_light->pos[1] = src_light->pos.y;
    dst_light->pos[2] = src_light->pos.z;
    dst_light->pos[3] = 0.0f;
    dst_light->color[0] = src_light->color.r / 255.0f;
    dst_light->color[1] = src_light->color.g / 255.0f;
    dst_light->color[2] = src_light->color.b / 255.0f;
    dst_light->color[3] = 0.0f;
    if (src_light->layout == LIGHT_LAYOUT_TR4) {
        dst_light->shade = src_light->u.tr4.intensity;
        dst_light->falloff = src_light->u.tr4.outer_radius;
    } else {
        dst_light->shade = src_light->u.legacy.shade.value_1;
        dst_light->falloff = src_light->u.legacy.falloff.value_1;
    }
    dst_light->kind = src_light->type;
}

static int16_t M_GetMinShade(void)
{
    switch (g_Config.rendering.lighting_contrast) {
    case LIGHTING_CONTRAST_LOW:
        return SHADE_NEUTRAL;
    case LIGHTING_CONTRAST_MEDIUM:
        return SHADE_HIGH;
    case LIGHTING_CONTRAST_HIGH:
        return 0;
    default:
        return SHADE_NEUTRAL;
    }
}

void Output_Uniforms_UploadOrthoMatrix(const OUTPUT_UNIFORMS *const uniforms)
{
    M_UNIFORM_MATRICES matrices = {};
    Output_GetOrthoProjectionMatrix(matrices.mat_proj);
    Output_FillMatrix(matrices.mat_view, &g_IDMatrix);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->matrices);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(matrices), &matrices);
}

void Output_Uniforms_UploadViewMatrix(
    const OUTPUT_UNIFORMS *const uniforms, const MATRIX *const matrix)
{
    M_UNIFORM_MATRICES matrices = {};
    Output_GetPerspProjectionMatrix(matrices.mat_proj);
    Output_FillMatrix(matrices.mat_view, matrix);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->matrices);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(matrices), &matrices);
}

void Output_Uniforms_UploadGeneral(const OUTPUT_UNIFORMS *const uniforms)
{
    M_UNIFORM_GENERAL general = {
        .time = Output_GetTime(),
        .time_in_game = Output_GetTimeInGame(),
        .brightness_multiplier = g_Config.visuals.game_brightness,
        .ui_brightness_multiplier = g_Config.visuals.ui_brightness,
        .gamma = g_Config.visuals.gamma,
        .sunset_duration = Output_GetSunsetDuration(),
        .tr_version = g_TRVersion,
        .uv_rotate_offset = Output_GetUVRotateOffset(),
        .viewport_size = {
            (float)Viewport_GetWidth(VIEWPORT_GAME),
            (float)Viewport_GetHeight(VIEWPORT_GAME),
        },
        .min_shade = M_GetMinShade(),
        .billboard_lock_mode = g_Config.rendering.sprite_lock_mode,
        .lighting_enabled = g_Config.rendering.enable_lighting,
        .textures_enabled = g_Config.rendering.enable_textures,
        .trapezoid_filter_enabled = g_Config.rendering.enable_trapezoid_filter,
        .reflections_enabled = g_Config.visuals.enable_reflections,
        .fog_distance = {Output_GetFogStart(), Output_GetFogEnd()},
        .fog_color = {
            Output_GetFogColor().r,
            Output_GetFogColor().g,
            Output_GetFogColor().b,
            Output_GetFogColor().a,
        },
    };

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->general);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(general), &general);
}

void Output_Uniforms_UploadFogDistance(
    const OUTPUT_UNIFORMS *const uniforms, const float start, const float end)
{
    ASSERT(uniforms != nullptr);
    const float fog_distance[2] = { start, end };
    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->general);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER,
        offsetof(M_UNIFORM_GENERAL, fog_distance), sizeof(fog_distance),
        &fog_distance);
}

void Output_Uniforms_UploadGameBrightnessMultiplier(
    const OUTPUT_UNIFORMS *const uniforms,
    const float game_brightness_multiplier)
{
    ASSERT(uniforms != nullptr);

    float clamped = game_brightness_multiplier;
    CLAMP(clamped, CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->general);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER,
        offsetof(M_UNIFORM_GENERAL, brightness_multiplier), sizeof(clamped),
        &clamped);
}

void Output_Uniforms_UploadUIBrightnessMultiplier(
    const OUTPUT_UNIFORMS *const uniforms, const float brightness_multiplier)
{
    ASSERT(uniforms != nullptr);

    float clamped = brightness_multiplier;
    CLAMP(clamped, CONFIG_MIN_BRIGHTNESS, CONFIG_MAX_BRIGHTNESS);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->general);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER,
        offsetof(M_UNIFORM_GENERAL, ui_brightness_multiplier), sizeof(clamped),
        &clamped);
}

void Output_Uniforms_UploadRoomLights(
    const OUTPUT_UNIFORMS *const uniforms, const ROOM *const room)
{
    M_UNIFORM_LIGHTS lights = {};

    // Only dynamic lights for now.
    M_UNIFORM_LIGHT *dst_light = lights.lights;
    if (room == nullptr) {
        lights.room_light_mode = RLM_SUNSET;
    } else {
        lights.room_light_mode = room->light_mode;
    }
    VECTOR *const dynamic_lights = Output_GetDynamicLights();
    for (int32_t i = 0; i < dynamic_lights->count; i++) {
        M_FillLight(dst_light, Vector_Get(dynamic_lights, i));
        dst_light++;
        if (dst_light - lights.lights >= M_MAX_LIGHTS) {
            break;
        }
    }

    lights.num_lights = dst_light - lights.lights;
    const size_t size = offsetof(M_UNIFORM_LIGHTS, lights)
        + lights.num_lights * sizeof(M_UNIFORM_LIGHT);

    M_PRIV *const p = uniforms->priv;
    if (memcmp(&p->last_lights, &lights, sizeof(lights)) == 0) {
        return;
    }
    memcpy(&p->last_lights, &lights, sizeof(lights));

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->lights);
    TRX_GL_TRACK_SUBDATA(glBufferSubData, GL_UNIFORM_BUFFER, 0, size, &lights);
}

OUTPUT_UNIFORMS *Output_Uniforms_Create(void)
{
    OUTPUT_UNIFORMS *const uniforms =
        Memory_Alloc(sizeof(OUTPUT_UNIFORMS) + sizeof(M_PRIV));
    glGenBuffers(4, &uniforms->general);
    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->general);
    glBufferData(
        GL_UNIFORM_BUFFER, sizeof(M_UNIFORM_GENERAL), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms->general);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->matrices);
    glBufferData(
        GL_UNIFORM_BUFFER, sizeof(M_UNIFORM_MATRICES), nullptr,
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uniforms->matrices);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->lights);
    glBufferData(
        GL_UNIFORM_BUFFER, sizeof(M_UNIFORM_LIGHTS), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, uniforms->lights);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->ls);
    glBufferData(
        GL_UNIFORM_BUFFER, Output_Lights_GetLSBufferSize(), nullptr,
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, uniforms->ls);
    TRX_GL_CheckError();

    Output_Lights_ResetUploadCache();
    uniforms->priv = (char *)uniforms + sizeof(OUTPUT_UNIFORMS);
    return uniforms;
}

void Output_Uniforms_Free(OUTPUT_UNIFORMS *const uniforms)
{
    if (uniforms == nullptr) {
        return;
    }
    if (uniforms->general != 0) {
        glDeleteBuffers(4, &uniforms->general);
        uniforms->general = 0;
        uniforms->matrices = 0;
        uniforms->lights = 0;
        uniforms->ls = 0;
    }
    Memory_Free(uniforms);
}
