#include <trx/game/output/shader.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/output.h>
#include <trx/game/viewport.h>
#include <trx/gfx/gl/program.h>
#include <trx/gfx/gl/utils.h>
#include <trx/memory.h>

typedef enum {
    M_UNIFORM_TEX_ATLAS,
    M_UNIFORM_TEX_ENV_MAP,
    M_UNIFORM_LIGHTING_MODE,
    M_UNIFORM_ALPHA_DISCARD,
    M_UNIFORM_GLOBAL_TINT,
    M_UNIFORM_PROJECTION_MATRIX,
    M_UNIFORM_VIEW_MODEL_MATRIX,
    M_UNIFORM_WIBBLE_EFFECT,
    M_UNIFORM_NUMBER_OF,
} M_UNIFORM;

#pragma pack(push, 1)
typedef struct {
    GLfloat mat_view[4][4];
    GLfloat fog_color[4];
    GLfloat fog_distance[2];
    GLfloat viewport_size[2];
    GLfloat time;
    GLfloat time_in_game;
    GLfloat brightness_multiplier;
    GLint billboard_lock_mode;
    GLint lighting_contrast;
    GLint trapezoid_filter_enabled;
    GLint reflections_enabled;
} M_UNIFORM_GLOBALS;
#pragma pack(pop)

struct OUTPUT_SHADER {
    GFX_GL_PROGRAM program;
    GLint uniforms[M_UNIFORM_NUMBER_OF];
    GLuint ubo_globals;

    bool is_wibble_effect;
    bool is_alpha_discard_enabled;
    RGB_F tint;
};

static void M_FillMatrix(GLfloat m[4][4], const MATRIX *const source)
{
    m[0][0] = source->_00 / (float)(1 << W2V_SHIFT);
    m[0][1] = source->_10 / (float)(1 << W2V_SHIFT);
    m[0][2] = source->_20 / (float)(1 << W2V_SHIFT);
    m[0][3] = 0.0;

    m[1][0] = source->_01 / (float)(1 << W2V_SHIFT);
    m[1][1] = source->_11 / (float)(1 << W2V_SHIFT);
    m[1][2] = source->_21 / (float)(1 << W2V_SHIFT);
    m[1][3] = 0.0;

    m[2][0] = source->_02 / (float)(1 << W2V_SHIFT);
    m[2][1] = source->_12 / (float)(1 << W2V_SHIFT);
    m[2][2] = source->_22 / (float)(1 << W2V_SHIFT);
    m[2][3] = 0.0;

    m[3][0] = source->_03 / (float)(1 << W2V_SHIFT);
    m[3][1] = source->_13 / (float)(1 << W2V_SHIFT);
    m[3][2] = source->_23 / (float)(1 << W2V_SHIFT);
    m[3][3] = 1.0;
}

static void M_UploadMatrix(
    const OUTPUT_SHADER *const shader, const M_UNIFORM target,
    const MATRIX *const source)
{
    GLfloat m[4][4];
    M_FillMatrix(m, source);

    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, shader->uniforms[target], 1, GL_FALSE, &m[0][0]);
}

OUTPUT_SHADER *Output_Shader_Create(const char *const path)
{
    OUTPUT_SHADER *const shader = Memory_Alloc(sizeof(OUTPUT_SHADER));

    GFX_GL_Program_Init(&shader->program);
    GFX_GL_Program_AttachShader(&shader->program, GL_VERTEX_SHADER, path);
    GFX_GL_Program_AttachShader(&shader->program, GL_FRAGMENT_SHADER, path);
    GFX_GL_Program_FragmentData(&shader->program, "outColor");
    GFX_GL_Program_Link(&shader->program);

    glGenBuffers(1, &shader->ubo_globals);
    glBindBuffer(GL_UNIFORM_BUFFER, shader->ubo_globals);
    glBufferData(
        GL_UNIFORM_BUFFER, sizeof(M_UNIFORM_GLOBALS), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, shader->ubo_globals);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    const char *const uniform_names[] = {
        [M_UNIFORM_TEX_ATLAS] = "uTexAtlas",
        [M_UNIFORM_TEX_ENV_MAP] = "uTexEnvMap",
        [M_UNIFORM_LIGHTING_MODE] = "uLightingMode",
        [M_UNIFORM_GLOBAL_TINT] = "uGlobalTint",
        [M_UNIFORM_PROJECTION_MATRIX] = "uMatProjection",
        [M_UNIFORM_VIEW_MODEL_MATRIX] = "uMatModelView",
        [M_UNIFORM_WIBBLE_EFFECT] = "uWibbleEffect",
        [M_UNIFORM_ALPHA_DISCARD] = "uDiscardAlpha",
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
    M_UNIFORM_GLOBALS globals = {
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
    M_FillMatrix(globals.mat_view, &g_W2VMatrix);

    glBindBuffer(GL_UNIFORM_BUFFER, shader->ubo_globals);
    GFX_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(globals), &globals);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
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

void Output_Shader_UploadAlphaDiscard(
    OUTPUT_SHADER *const shader, const bool is_enabled)
{
    if (is_enabled == shader->is_alpha_discard_enabled) {
        return;
    }
    GFX_TRACK_UNIFORM(
        glUniform1i, shader->uniforms[M_UNIFORM_ALPHA_DISCARD], is_enabled);
    shader->is_alpha_discard_enabled = is_enabled;
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
