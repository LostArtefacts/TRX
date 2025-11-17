#define M_MAX_LIGHTS 32

#include <trx/game/output/uniforms.h>

#include <trx/config.h>
#include <trx/game/output.h>
#include <trx/game/output/utils.h>
#include <trx/game/rooms.h>
#include <trx/gfx/gl/utils.h>
#include <trx/log.h>
#include <trx/memory.h>
#include <trx/utils.h>
#include <trx/vector.h>

#define M_GLOBAL_MEMBERS                                                       \
    X_DECLARE_MEMBER(float, fog_color, [4])                                    \
    X_DECLARE_MEMBER(float, fog_distance, [2])                                 \
    X_DECLARE_MEMBER(float, viewport_size, [2])                                \
    X_DECLARE_MEMBER(float, time)                                              \
    X_DECLARE_MEMBER(float, time_in_game)                                      \
    X_DECLARE_MEMBER(float, brightness_multiplier)                             \
    X_DECLARE_MEMBER(float, sunset_duration)                                   \
    X_DECLARE_MEMBER(int, billboard_lock_mode)                                 \
    X_DECLARE_MEMBER(int, lighting_contrast)                                   \
    X_DECLARE_MEMBER(int, lighting_enabled)                                    \
    X_DECLARE_MEMBER(int, trapezoid_filter_enabled)                            \
    X_DECLARE_MEMBER(int, reflections_enabled)                                 \
    X_DECLARE_MEMBER(int, use_new_lighting)

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
    int shade;
    int falloff;
    int _pad[2];
} M_UNIFORM_LIGHT;

typedef struct {
    int num_lights;
    int room_light_mode;
    int _pad[2];
    M_UNIFORM_LIGHT lights[M_MAX_LIGHTS];
} M_UNIFORM_LIGHTS;

typedef struct {
    float adder;
    float divider;
    float _pad[2];
    float vector_view[4];
} M_UNIFORM_LS;
#pragma pack(pop)

static void M_FillLight(
    M_UNIFORM_LIGHT *const dst_light, const LIGHT *const src_light)
{
    dst_light->pos[0] = src_light->pos.x;
    dst_light->pos[1] = src_light->pos.y;
    dst_light->pos[2] = src_light->pos.z;
    dst_light->pos[3] = 0.0f;
    dst_light->shade = src_light->shade.value_1;
    dst_light->falloff = src_light->falloff.value_1;
}

void Output_Uniforms_UploadOrthoMatrix(const OUTPUT_UNIFORMS *const uniforms)
{
    M_UNIFORM_MATRICES matrices = {};
    Output_GetOrthoProjectionMatrix(matrices.mat_proj);
    Output_FillMatrix(matrices.mat_view, &g_IDMatrix);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->matrices);
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(matrices), &matrices);
}

void Output_Uniforms_UploadViewMatrix(
    const OUTPUT_UNIFORMS *const uniforms, const MATRIX *const matrix)
{
    M_UNIFORM_MATRICES matrices = {};
    Output_GetPerspProjectionMatrix(matrices.mat_proj);
    Output_FillMatrix(matrices.mat_view, matrix);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->matrices);
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(matrices), &matrices);
}

void Output_Uniforms_UploadGeneral(const OUTPUT_UNIFORMS *const uniforms)
{
    M_UNIFORM_GENERAL general = {
        .time = Output_GetTime(),
        .time_in_game = Output_GetTimeInGame(),
        .brightness_multiplier = g_Config.visuals.brightness,
        .sunset_duration = Output_GetSunsetDuration(),
        .viewport_size = {
            (float)Viewport_GetWidth(VIEWPORT_GAME),
            (float)Viewport_GetHeight(VIEWPORT_GAME),
        },
        .billboard_lock_mode = g_Config.rendering.sprite_lock_mode,
        .lighting_contrast = g_Config.rendering.lighting_contrast,
        .lighting_enabled = g_Config.rendering.enable_lighting,
        .trapezoid_filter_enabled = g_Config.rendering.enable_trapezoid_filter,
        .reflections_enabled = g_Config.visuals.enable_reflections,
        .use_new_lighting = g_Config.debug.use_new_lighting,
        .fog_distance = {Output_GetFogStart(), Output_GetFogEnd()},
        .fog_color = {
            Output_GetFogColor().r,
            Output_GetFogColor().g,
            Output_GetFogColor().b,
            Output_GetFogColor().a,
        },
    };

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->general);
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(general), &general);
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

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->lights);
    GFX_TRACK_SUBDATA(glBufferSubData, GL_UNIFORM_BUFFER, 0, size, &lights);
}

void Output_Uniforms_UploadCPULight(
    const OUTPUT_UNIFORMS *const uniforms, const OUTPUT_LIGHT_INFO *const info)
{
    M_UNIFORM_LS ls = {};
    ls.adder = info->ls_adder;
    ls.divider = info->ls_divider / (float)(1 << (W2V_SHIFT));
    ls.vector_view[0] = info->ls_vector_view.x;
    ls.vector_view[1] = info->ls_vector_view.y;
    ls.vector_view[2] = info->ls_vector_view.z;
    ls.vector_view[3] = 0;

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->ls);
    GFX_TRACK_SUBDATA(glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(ls), &ls);
    GFX_GL_CheckError();
}

OUTPUT_UNIFORMS *Output_Uniforms_Create(void)
{
    OUTPUT_UNIFORMS *const uniforms = Memory_Alloc(sizeof(OUTPUT_UNIFORMS));
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
        GL_UNIFORM_BUFFER, sizeof(M_UNIFORM_LS), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, uniforms->ls);
    GFX_GL_CheckError();

    return uniforms;
}

void Output_Uniforms_Free(OUTPUT_UNIFORMS *const uniforms)
{
    Memory_Free(uniforms);
}
