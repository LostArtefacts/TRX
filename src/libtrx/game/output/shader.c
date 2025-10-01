#include "game/output/shader.h"

#include "config.h"
#include "debug.h"
#include "game/output.h"
#include "game/viewport.h"
#include "gfx/gl/program.h"
#include "gfx/gl/utils.h"
#include "memory.h"

typedef enum {
    M_UNIFORM_TIME,
    M_UNIFORM_TIME_IN_GAME,
    M_UNIFORM_TEX_ATLAS,
    M_UNIFORM_TEX_ENV_MAP,
    M_UNIFORM_SMOOTHING_ENABLED,
    M_UNIFORM_TRAPEZOID_FILTER_ENABLED,
    M_UNIFORM_LIGHTING_MODE,
    M_UNIFORM_LIGHTING_CONTRAST,
    M_UNIFORM_REFLECTIONS_ENABLED,
    M_UNIFORM_BRIGHTNESS_MULTIPLIER,
    M_UNIFORM_GLOBAL_TINT,
    M_UNIFORM_FOG_COLOR,
    M_UNIFORM_FOG_DISTANCE,
    M_UNIFORM_VIEWPORT_SIZE,
    M_UNIFORM_PROJECTION_MATRIX,
    M_UNIFORM_VIEW_MATRIX,
    M_UNIFORM_VIEW_MODEL_MATRIX,
    M_UNIFORM_WIBBLE_EFFECT,
    M_UNIFORM_BILLBOARD_LOCK_MODE,
    M_UNIFORM_NUMBER_OF,
} M_UNIFORM;

struct OUTPUT_SHADER {
    GFX_GL_PROGRAM program;
    GLint uniforms[M_UNIFORM_NUMBER_OF];

    bool is_wibble_effect;
    RGB_F tint;
};

static void M_UploadMatrix(
    const OUTPUT_SHADER *const shader, const M_UNIFORM target,
    const MATRIX *const source)
{
    GLfloat m[4][4];
    m[0][0] = source->_00 / (float)(1 << W2V_SHIFT);
    m[0][1] = source->_01 / (float)(1 << W2V_SHIFT);
    m[0][2] = source->_02 / (float)(1 << W2V_SHIFT);
    m[0][3] = source->_03 / (float)(1 << W2V_SHIFT);

    m[1][0] = source->_10 / (float)(1 << W2V_SHIFT);
    m[1][1] = source->_11 / (float)(1 << W2V_SHIFT);
    m[1][2] = source->_12 / (float)(1 << W2V_SHIFT);
    m[1][3] = source->_13 / (float)(1 << W2V_SHIFT);

    m[2][0] = source->_20 / (float)(1 << W2V_SHIFT);
    m[2][1] = source->_21 / (float)(1 << W2V_SHIFT);
    m[2][2] = source->_22 / (float)(1 << W2V_SHIFT);
    m[2][3] = source->_23 / (float)(1 << W2V_SHIFT);

    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = 1.0;

    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, shader->uniforms[target], 1, GL_TRUE, &m[0][0]);
}

OUTPUT_SHADER *Output_Shader_Create(const char *const path)
{
    OUTPUT_SHADER *const shader = Memory_Alloc(sizeof(OUTPUT_SHADER));

    GFX_GL_Program_Init(&shader->program);
    GFX_GL_Program_AttachShader(&shader->program, GL_VERTEX_SHADER, path);
    GFX_GL_Program_AttachShader(&shader->program, GL_FRAGMENT_SHADER, path);
    GFX_GL_Program_FragmentData(&shader->program, "outColor");
    GFX_GL_Program_Link(&shader->program);

    const char *const uniform_names[] = {
        [M_UNIFORM_TIME] = "uTime",
        [M_UNIFORM_TIME_IN_GAME] = "uTimeInGame",
        [M_UNIFORM_TEX_ATLAS] = "uTexAtlas",
        [M_UNIFORM_TEX_ENV_MAP] = "uTexEnvMap",
        [M_UNIFORM_SMOOTHING_ENABLED] = "uSmoothingEnabled",
        [M_UNIFORM_TRAPEZOID_FILTER_ENABLED] = "uTrapezoidFilterEnabled",
        [M_UNIFORM_LIGHTING_MODE] = "uLightingMode",
        [M_UNIFORM_LIGHTING_CONTRAST] = "uLightingContrast",
        [M_UNIFORM_REFLECTIONS_ENABLED] = "uReflectionsEnabled",
        [M_UNIFORM_BRIGHTNESS_MULTIPLIER] = "uBrightnessMultiplier",
        [M_UNIFORM_GLOBAL_TINT] = "uGlobalTint",
        [M_UNIFORM_FOG_COLOR] = "uFogColor",
        [M_UNIFORM_FOG_DISTANCE] = "uFogDistance",
        [M_UNIFORM_VIEWPORT_SIZE] = "uViewportSize",
        [M_UNIFORM_PROJECTION_MATRIX] = "uMatProjection",
        [M_UNIFORM_VIEW_MATRIX] = "uMatView",
        [M_UNIFORM_VIEW_MODEL_MATRIX] = "uMatModelView",
        [M_UNIFORM_WIBBLE_EFFECT] = "uWibbleEffect",
        [M_UNIFORM_BILLBOARD_LOCK_MODE] = "uBillboardLockMode",
    };
    for (int32_t i = 0; i < M_UNIFORM_NUMBER_OF; i++) {
        shader->uniforms[i] =
            GFX_GL_Program_UniformLocation(&shader->program, uniform_names[i]);
        GFX_GL_CheckError();
    }

    GFX_GL_Program_Bind(&shader->program);
    glUniform1i(shader->uniforms[M_UNIFORM_TEX_ATLAS], 0);
    glUniform1i(shader->uniforms[M_UNIFORM_TEX_ENV_MAP], 1);
    return shader;
}

void Output_Shader_Free(OUTPUT_SHADER *const shader)
{
    GFX_GL_Program_Close(&shader->program);
    Memory_Free(shader);
}

void Output_Shader_Bind(const OUTPUT_SHADER *const shader)
{
    ASSERT(shader != nullptr);
    GFX_GL_Program_Bind(&shader->program);
}

void Output_Shader_UploadCommonUniforms(const OUTPUT_SHADER *const shader)
{
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_TRAPEZOID_FILTER_ENABLED],
        g_Config.rendering.enable_trapezoid_filter);
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_LIGHTING_MODE],
        g_Config.rendering.enable_lighting);
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_LIGHTING_CONTRAST],
        g_Config.rendering.lighting_contrast);
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_REFLECTIONS_ENABLED],
        g_Config.visuals.enable_reflections);
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_BRIGHTNESS_MULTIPLIER],
        g_Config.visuals.brightness);
    GFX_TRACK_UNIFORM(
        glUniform2f, shader->uniforms[M_UNIFORM_VIEWPORT_SIZE],
        Viewport_GetWidth(VIEWPORT_GAME), Viewport_GetHeight(VIEWPORT_GAME));
    GFX_TRACK_UNIFORM(
        glUniform4f, shader->uniforms[M_UNIFORM_FOG_COLOR],
        Output_GetFogColor().r, Output_GetFogColor().g, Output_GetFogColor().b,
        Output_GetFogColor().a);
    GFX_TRACK_UNIFORM(
        glUniform2f, shader->uniforms[M_UNIFORM_FOG_DISTANCE],
        Output_GetFogStart(), Output_GetFogEnd());
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_TIME], Output_GetTime());
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_TIME_IN_GAME],
        Output_GetTimeInGame());
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_BILLBOARD_LOCK_MODE],
        g_Config.rendering.sprite_lock_mode);
    Output_Shader_UploadViewMatrix(shader);
}

void Output_Shader_UploadViewMatrix(const OUTPUT_SHADER *const shader)
{
    M_UploadMatrix(shader, M_UNIFORM_VIEW_MATRIX, &g_W2VMatrix);
}

void Output_Shader_UploadViewModelMatrix(
    const OUTPUT_SHADER *const shader, const MATRIX *const source)
{
    M_UploadMatrix(shader, M_UNIFORM_VIEW_MODEL_MATRIX, source);
}

void Output_Shader_UploadPerspProjectionMatrix(
    const OUTPUT_SHADER *const shader)
{
    GLfloat projection[4][4];
    Output_GetPerspProjectionMatrix(projection);
    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, shader->uniforms[M_UNIFORM_PROJECTION_MATRIX], 1,
        GL_TRUE, &projection[0][0]);
}

void Output_Shader_UploadOrthoProjectionMatrix(
    const OUTPUT_SHADER *const shader)
{
    GLfloat projection[4][4];
    Output_GetOrthoProjectionMatrix(projection);
    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, shader->uniforms[M_UNIFORM_PROJECTION_MATRIX], 1,
        GL_TRUE, &projection[0][0]);
}

void Output_Shader_UploadWibbleEffect(
    OUTPUT_SHADER *const shader, const bool is_enabled)
{
    if (is_enabled == shader->is_wibble_effect) {
        return;
    }
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_WIBBLE_EFFECT], is_enabled);
    shader->is_wibble_effect = is_enabled;
}

void Output_Shader_UploadTint(OUTPUT_SHADER *const shader, const RGB_F tint)
{
    if (tint.r == shader->tint.r && tint.g == shader->tint.g
        && tint.b == shader->tint.b) {
        return;
    }
    GFX_TRACK_UNIFORM(
        glUniform3f, shader->uniforms[M_UNIFORM_GLOBAL_TINT], tint.r, tint.g,
        tint.b);
    shader->tint = tint;
}

void Output_Shader_UploadLightingMode(
    const OUTPUT_SHADER *const shader, const LIGHTING_MODE mode)
{
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_LIGHTING_MODE], mode);
}

void Output_Shader_UploadSmoothingEnabled(
    const OUTPUT_SHADER *const shader, const bool is_enabled)
{
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_SMOOTHING_ENABLED], is_enabled);
}
