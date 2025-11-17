#include <trx/game/output/uniforms.h>

#include <trx/config.h>
#include <trx/game/output.h>
#include <trx/game/output/utils.h>
#include <trx/game/rooms.h>
#include <trx/gfx/gl/utils.h>
#include <trx/log.h>
#include <trx/memory.h>
#include <trx/vector.h>

#define M_GLOBAL_MEMBERS                                                       \
    X_DECLARE_MEMBER(float, fog_color, [4])                                    \
    X_DECLARE_MEMBER(float, fog_distance, [2])                                 \
    X_DECLARE_MEMBER(float, viewport_size, [2])                                \
    X_DECLARE_MEMBER(float, time)                                              \
    X_DECLARE_MEMBER(float, time_in_game)                                      \
    X_DECLARE_MEMBER(float, brightness_multiplier)                             \
    X_DECLARE_MEMBER(int, billboard_lock_mode)                                 \
    X_DECLARE_MEMBER(int, lighting_contrast)                                   \
    X_DECLARE_MEMBER(int, trapezoid_filter_enabled)                            \
    X_DECLARE_MEMBER(int, reflections_enabled)

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
#pragma pack(pop)

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
        .viewport_size = {
            (float)Viewport_GetWidth(VIEWPORT_GAME),
            (float)Viewport_GetHeight(VIEWPORT_GAME),
        },
        .billboard_lock_mode = g_Config.rendering.sprite_lock_mode,
        .lighting_contrast = g_Config.rendering.lighting_contrast,
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
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(general), &general);
}

OUTPUT_UNIFORMS *Output_Uniforms_Create(void)
{
    OUTPUT_UNIFORMS *const uniforms = Memory_Alloc(sizeof(OUTPUT_UNIFORMS));
    glGenBuffers(2, &uniforms->general);
    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->general);
    glBufferData(
        GL_UNIFORM_BUFFER, sizeof(M_UNIFORM_GENERAL), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms->general);

    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->matrices);
    glBufferData(
        GL_UNIFORM_BUFFER, sizeof(M_UNIFORM_MATRICES), nullptr,
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uniforms->matrices);

    return uniforms;
}

void Output_Uniforms_Free(OUTPUT_UNIFORMS *const uniforms)
{
    Memory_Free(uniforms);
}
