#include "game/output/shader.h"

#include "game/output.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

typedef enum {
    M_UNIFORM_TEX_ATLAS,
    M_UNIFORM_TEX_UVW,
    M_UNIFORM_SMOOTHING_ENABLED,
    M_UNIFORM_TRAPEZOID_FILTER_ENABLED,
    M_UNIFORM_BRIGHTNESS_MULTIPLIER,
    M_UNIFORM_GLOBAL_TINT,
    M_UNIFORM_VIEWPORT_SIZE,
    M_UNIFORM_PROJECTION_MATRIX,
    M_UNIFORM_MODEL_MATRIX,
    M_UNIFORM_WIBBLE_OFFSET,
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
        [M_UNIFORM_TEX_ATLAS] = "uTexture",
        [M_UNIFORM_TEX_UVW] = "uUVW",
        [M_UNIFORM_SMOOTHING_ENABLED] = "uSmoothingEnabled",
        [M_UNIFORM_TRAPEZOID_FILTER_ENABLED] = "uTrapezoidFilterEnabled",
        [M_UNIFORM_BRIGHTNESS_MULTIPLIER] = "uBrightnessMultiplier",
        [M_UNIFORM_GLOBAL_TINT] = "uGlobalTint",
        [M_UNIFORM_VIEWPORT_SIZE] = "uViewportSize",
        [M_UNIFORM_PROJECTION_MATRIX] = "uMatProjection",
        [M_UNIFORM_MODEL_MATRIX] = "uMatModelView",
        [M_UNIFORM_WIBBLE_OFFSET] = "uWibbleOffset",
    };
    for (int32_t i = 0; i < M_UNIFORM_NUMBER_OF; i++) {
        shader->uniforms[i] =
            GFX_GL_Program_UniformLocation(&shader->program, uniform_names[i]);
        GFX_GL_CheckError();
    }

    GFX_GL_Program_Bind(&shader->program);
    glUniform1i(shader->uniforms[M_UNIFORM_TEX_ATLAS], 0);
    glUniform1i(shader->uniforms[M_UNIFORM_TEX_UVW], 1);
    return shader;
}

void Output_Shader_Free(OUTPUT_SHADER *const shader)
{
    GFX_GL_Program_Close(&shader->program);
    Memory_Free(shader);
}

void Output_Shader_UploadCommonUniforms(const OUTPUT_SHADER *const shader)
{
    GFX_GL_Program_Bind(&shader->program);
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_SMOOTHING_ENABLED],
        g_Config.rendering.texture_filter);
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_TRAPEZOID_FILTER_ENABLED],
        g_Config.rendering.enable_trapezoid_filter);
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_BRIGHTNESS_MULTIPLIER],
        g_Config.visuals.brightness);
    GFX_TRACK_UNIFORM(
        glUniform2f, shader->uniforms[M_UNIFORM_VIEWPORT_SIZE],
        GFX_Context_GetDisplayWidth(), GFX_Context_GetDisplayHeight());
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

void Output_Shader_UploadWibble(
    const OUTPUT_SHADER *const shader, const int32_t offset)
{
    GFX_TRACK_UNIFORM(
        glUniform1f, shader->uniforms[M_UNIFORM_WIBBLE_OFFSET], offset);
}

void Output_Shader_UploadTint(
    const OUTPUT_SHADER *const shader, const RGB_F tint)
{
    GFX_GL_Program_Bind(&shader->program);
    GFX_TRACK_UNIFORM(
        glUniform3f, shader->uniforms[M_UNIFORM_GLOBAL_TINT], tint.r, tint.g,
        tint.b);
}
