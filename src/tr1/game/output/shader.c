#include "game/output/shader.h"

#include "game/output.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

typedef enum {
    M_UNIFORM_TIME,
    M_UNIFORM_TEX_ATLAS,
    M_UNIFORM_TEX_ATLAS_SIZES,
    M_UNIFORM_TEX_UVW,
    M_UNIFORM_TEX_ENV_MAP,
    M_UNIFORM_SMOOTHING_ENABLED,
    M_UNIFORM_ALPHA_DISCARD_ENABLED,
    M_UNIFORM_ALPHA_THRESHOLD,
    M_UNIFORM_TRAPEZOID_FILTER_ENABLED,
    M_UNIFORM_REFLECTIONS_ENABLED,
    M_UNIFORM_BRIGHTNESS_MULTIPLIER,
    M_UNIFORM_GLOBAL_TINT,
    M_UNIFORM_FOG,
    M_UNIFORM_VIEWPORT_SIZE,
    M_UNIFORM_PROJECTION_MATRIX,
    M_UNIFORM_MODEL_MATRIX,
    M_UNIFORM_WIBBLE_EFFECT,
    M_UNIFORM_WATER_EFFECT,
    M_UNIFORM_NUMBER_OF,
} M_UNIFORM;

struct OUTPUT_SHADER {
    GFX_GL_PROGRAM program;
    GLint uniforms[M_UNIFORM_NUMBER_OF];
};

OUTPUT_SHADER *Output_Shader_Create(const char *const path)
{
    OUTPUT_SHADER *const shader = Memory_Alloc(sizeof(OUTPUT_SHADER));

    GFX_GL_Program_Init(&shader->program);
    GFX_GL_Program_AttachShader(
        &shader->program, GL_VERTEX_SHADER, path,
        GFX_Context_GetConfig()->backend);
    GFX_GL_Program_AttachShader(
        &shader->program, GL_FRAGMENT_SHADER, path,
        GFX_Context_GetConfig()->backend);
    GFX_GL_Program_FragmentData(&shader->program, "outColor");
    GFX_GL_Program_Link(&shader->program);

    const char *const uniform_names[] = {
        [M_UNIFORM_TIME] = "uTime",
        [M_UNIFORM_TEX_ATLAS] = "uTexAtlas",
        [M_UNIFORM_TEX_ATLAS_SIZES] = "uAtlasSizes",
        [M_UNIFORM_TEX_UVW] = "uUVW",
        [M_UNIFORM_TEX_ENV_MAP] = "uTexEnvMap",
        [M_UNIFORM_SMOOTHING_ENABLED] = "uSmoothingEnabled",
        [M_UNIFORM_ALPHA_DISCARD_ENABLED] = "uAlphaDiscardEnabled",
        [M_UNIFORM_ALPHA_THRESHOLD] = "uAlphaThreshold",
        [M_UNIFORM_TRAPEZOID_FILTER_ENABLED] = "uTrapezoidFilterEnabled",
        [M_UNIFORM_REFLECTIONS_ENABLED] = "uReflectionsEnabled",
        [M_UNIFORM_BRIGHTNESS_MULTIPLIER] = "uBrightnessMultiplier",
        [M_UNIFORM_GLOBAL_TINT] = "uGlobalTint",
        [M_UNIFORM_FOG] = "uFog",
        [M_UNIFORM_VIEWPORT_SIZE] = "uViewportSize",
        [M_UNIFORM_PROJECTION_MATRIX] = "uMatProjection",
        [M_UNIFORM_MODEL_MATRIX] = "uMatModelView",
        [M_UNIFORM_WIBBLE_EFFECT] = "uWibbleEffect",
        [M_UNIFORM_WATER_EFFECT] = "uWaterEffect",
    };
    for (int32_t i = 0; i < M_UNIFORM_NUMBER_OF; i++) {
        shader->uniforms[i] =
            GFX_GL_Program_UniformLocation(&shader->program, uniform_names[i]);
        GFX_GL_CheckError();
    }

    GFX_GL_Program_Bind(&shader->program);
    glUniform1i(shader->uniforms[M_UNIFORM_TEX_ATLAS], 0);
    glUniform1i(shader->uniforms[M_UNIFORM_TEX_UVW], 1);
    glUniform1i(shader->uniforms[M_UNIFORM_TEX_ENV_MAP], 2);
    glUniform1i(shader->uniforms[M_UNIFORM_TEX_ATLAS_SIZES], 3);
    return shader;
}

void Output_Shader_Free(OUTPUT_SHADER *const shader)
{
    GFX_GL_Program_Close(&shader->program);
    Memory_Free(shader);
}

void Output_Shader_Bind(const OUTPUT_SHADER *const shader)
{
    GFX_GL_Program_Bind(&shader->program);
}

void Output_Shader_UploadCommonUniforms(const OUTPUT_SHADER *const shader)
{
    GFX_GL_Program_Bind(&shader->program);

    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_SMOOTHING_ENABLED],
        g_Config.rendering.texture_filter);
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_ALPHA_THRESHOLD],
        g_Config.rendering.enable_wireframe ? -1.0f : 0.0f);
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_ALPHA_DISCARD_ENABLED],
        !g_Config.rendering.enable_wireframe);
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_TRAPEZOID_FILTER_ENABLED],
        g_Config.rendering.enable_trapezoid_filter);
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_REFLECTIONS_ENABLED],
        g_Config.visuals.enable_reflections);
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_BRIGHTNESS_MULTIPLIER],
        g_Config.visuals.brightness);
    GFX_TRACK_UNIFORM(
        glUniform2f, shader->uniforms[M_UNIFORM_VIEWPORT_SIZE],
        GFX_Context_GetDisplayWidth(), GFX_Context_GetDisplayHeight());
    GFX_TRACK_UNIFORM(
        glUniform2f, shader->uniforms[M_UNIFORM_FOG], Output_GetDrawDistFade(),
        Output_GetDrawDistMax());
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_TIME], Output_GetTime());
}

void Output_Shader_UploadMatrix(
    const OUTPUT_SHADER *const shader, const MATRIX *const source)
{
    GLfloat target[4][4];
    target[0][0] = source->_00 / (float)(1 << W2V_SHIFT);
    target[0][1] = source->_01 / (float)(1 << W2V_SHIFT);
    target[0][2] = source->_02 / (float)(1 << W2V_SHIFT);
    target[0][3] = source->_03 / (float)(1 << W2V_SHIFT);

    target[1][0] = source->_10 / (float)(1 << W2V_SHIFT);
    target[1][1] = source->_11 / (float)(1 << W2V_SHIFT);
    target[1][2] = source->_12 / (float)(1 << W2V_SHIFT);
    target[1][3] = source->_13 / (float)(1 << W2V_SHIFT);

    target[2][0] = source->_20 / (float)(1 << W2V_SHIFT);
    target[2][1] = source->_21 / (float)(1 << W2V_SHIFT);
    target[2][2] = source->_22 / (float)(1 << W2V_SHIFT);
    target[2][3] = source->_23 / (float)(1 << W2V_SHIFT);

    target[3][0] = 0.0;
    target[3][1] = 0.0;
    target[3][2] = 0.0;
    target[3][3] = 1.0;

    GFX_GL_Program_Bind(&shader->program);
    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, shader->uniforms[M_UNIFORM_MODEL_MATRIX], 1,
        GL_TRUE, &target[0][0]);
}

void Output_Shader_UploadProjectionMatrix(const OUTPUT_SHADER *const shader)
{
    GFX_GL_Program_Bind(&shader->program);

    GLfloat projection[4][4];
    Output_GetProjectionMatrix(projection);
    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, shader->uniforms[M_UNIFORM_PROJECTION_MATRIX], 1,
        GL_TRUE, &projection[0][0]);
}

void Output_Shader_UploadWibbleEffect(
    const OUTPUT_SHADER *const shader, const bool is_enabled)
{
    GFX_GL_Program_Bind(&shader->program);
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_WIBBLE_EFFECT], is_enabled);
}

void Output_Shader_UploadWaterEffect(
    const OUTPUT_SHADER *const shader, const bool is_enabled)
{
    GFX_GL_Program_Bind(&shader->program);
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_WATER_EFFECT], is_enabled);
}

void Output_Shader_UploadTint(
    const OUTPUT_SHADER *const shader, const RGB_F tint)
{
    GFX_GL_Program_Bind(&shader->program);
    GFX_TRACK_UNIFORM(
        glUniform3f, shader->uniforms[M_UNIFORM_GLOBAL_TINT], tint.r, tint.g,
        tint.b);
}
