#include "game/output/sprite_program.h"

#include "game/output.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/gfx/gl/utils.h>

typedef enum {
    M_UNIFORM_TEX_ATLAS,
    M_UNIFORM_TEX_UVW,
    M_UNIFORM_SMOOTHING_ENABLED,
    M_UNIFORM_BRIGHTNESS_MULTIPLIER,
    M_UNIFORM_GLOBAL_TINT,
    M_UNIFORM_VIEWPORT_SIZE,
    M_UNIFORM_PROJECTION_MATRIX,
    M_UNIFORM_MODEL_MATRIX,
    M_UNIFORM_WIBBLE_OFFSET,
    M_UNIFORM_NUMBER_OF,
} M_UNIFORM;

static GFX_GL_PROGRAM m_Program;
static GLint m_Uniforms[M_UNIFORM_NUMBER_OF];

void Output_SpriteProgram_Init(void)
{
    GFX_GL_Program_Init(&m_Program);
    GFX_GL_Program_AttachShader(
        &m_Program, GL_VERTEX_SHADER, "shaders/sprites.glsl",
        GFX_Context_GetConfig()->backend);
    GFX_GL_Program_AttachShader(
        &m_Program, GL_FRAGMENT_SHADER, "shaders/sprites.glsl",
        GFX_Context_GetConfig()->backend);
    GFX_GL_Program_FragmentData(&m_Program, "outColor");
    GFX_GL_Program_Link(&m_Program);

    const char *const uniform_names[] = {
        [M_UNIFORM_TEX_ATLAS] = "uTexture",
        [M_UNIFORM_TEX_UVW] = "uUVW",
        [M_UNIFORM_SMOOTHING_ENABLED] = "uSmoothingEnabled",
        [M_UNIFORM_BRIGHTNESS_MULTIPLIER] = "uBrightnessMultiplier",
        [M_UNIFORM_GLOBAL_TINT] = "uGlobalTint",
        [M_UNIFORM_VIEWPORT_SIZE] = "uViewportSize",
        [M_UNIFORM_PROJECTION_MATRIX] = "uMatProjection",
        [M_UNIFORM_MODEL_MATRIX] = "uMatModelView",
        [M_UNIFORM_WIBBLE_OFFSET] = "uWibbleOffset",
    };
    for (int32_t i = 0; i < M_UNIFORM_NUMBER_OF; i++) {
        m_Uniforms[i] =
            GFX_GL_Program_UniformLocation(&m_Program, uniform_names[i]);
        GFX_GL_CheckError();
    }

    GFX_GL_Program_Bind(&m_Program);
    glUniform1i(m_Uniforms[M_UNIFORM_TEX_ATLAS], 0);
    glUniform1i(m_Uniforms[M_UNIFORM_TEX_UVW], 1);
}

void Output_SpriteProgram_Shutdown(void)
{
    GFX_GL_Program_Close(&m_Program);
}

void Output_SpriteProgram_Bind(void)
{
    GFX_GL_Program_Bind(&m_Program);
}

void Output_SpriteProgram_UploadCommonUniforms(void)
{
    Output_SpriteProgram_Bind();
    GFX_TRACK_UNIFORM(
        glUniform1f, m_Uniforms[M_UNIFORM_SMOOTHING_ENABLED],
        g_Config.rendering.texture_filter);
    GFX_TRACK_UNIFORM(
        glUniform1f, m_Uniforms[M_UNIFORM_BRIGHTNESS_MULTIPLIER],
        g_Config.visuals.brightness);
    GFX_TRACK_UNIFORM(
        glUniform2f, m_Uniforms[M_UNIFORM_VIEWPORT_SIZE],
        GFX_Context_GetDisplayWidth(), GFX_Context_GetDisplayHeight());
    GFX_TRACK_UNIFORM(
        glUniform1f, m_Uniforms[M_UNIFORM_WIBBLE_OFFSET],
        Output_GetWibbleOffset());
}

void Output_SpriteProgram_UploadMatrix(const MATRIX *const source)
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

    Output_SpriteProgram_Bind();
    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, m_Uniforms[M_UNIFORM_MODEL_MATRIX], 1, GL_TRUE,
        &target[0][0]);
}

void Output_SpriteProgram_UploadProjectionMatrix(void)
{
    Output_SpriteProgram_Bind();

    GLfloat projection[4][4];
    Output_GetProjectionMatrix(projection);
    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, m_Uniforms[M_UNIFORM_PROJECTION_MATRIX], 1, GL_TRUE,
        &projection[0][0]);
}

void Output_SpriteProgram_UploadTint(const RGB_F tint)
{
    GFX_GL_Program_Bind(&m_Program);
    GFX_TRACK_UNIFORM(
        glUniform3f, m_Uniforms[M_UNIFORM_GLOBAL_TINT], tint.r, tint.g, tint.b);
}
