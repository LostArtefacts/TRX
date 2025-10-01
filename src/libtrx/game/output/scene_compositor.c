#include "game/output/scene_compositor.h"

#include "config.h"
#include "debug.h"
#include "game/output.h"
#include "game/output/shader.h"
#include "game/output/textures.h"
#include "game/shell.h"
#include "gfx/context.h"
#include "vector.h"

#define M_PROCESS_SOURCES(p, func, ...)                                        \
    do {                                                                       \
        for (int32_t i = 0; i < p->sources->count; i++) {                      \
            const SCENE_SOURCE *const source =                                 \
                *(SCENE_SOURCE **)Vector_Get(p->sources, i);                   \
            if (source->func != nullptr) {                                     \
                source->func(source, ##__VA_ARGS__);                           \
            }                                                                  \
        }                                                                      \
    } while (0)

typedef struct {
    VECTOR *sources;
    GLuint sampler_id;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_SetSamplerFilter(
    const GLuint sampler, const GFX_TEXTURE_FILTER filter)
{
    const GLenum gl_filter = filter == GFX_TF_BILINEAR ? GL_LINEAR : GL_NEAREST;
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, gl_filter);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, gl_filter);
}

static void M_SetupShaderForScene(
    OUTPUT_SHADER *const shader, const GFX_TEXTURE_FILTER filter,
    const bool lighting)
{
    Output_Shader_UploadSmoothingEnabled(shader, filter == GFX_TF_BILINEAR);
    Output_Shader_UploadPerspProjectionMatrix(shader);
    Output_Shader_UploadLightingMode(
        shader, lighting ? LIGHTING_MODE_FULL : LIGHTING_MODE_OFF);
}

static void M_SetupShaderForUI(
    OUTPUT_SHADER *const shader, const GFX_TEXTURE_FILTER filter)
{
    Output_Shader_UploadSmoothingEnabled(shader, filter == GFX_TF_BILINEAR);
    Output_Shader_UploadOrthoProjectionMatrix(shader);
    Output_Shader_UploadLightingMode(shader, LIGHTING_MODE_ONLY_SHADES);
}

static void M_BindTextures(const M_PRIV *p)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, Output_Textures_GetAtlasTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Output_Textures_GetEnvMapTexture());
}

static void M_SetBlendModeForScene(bool wireframe)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, wireframe ? GL_ZERO : GL_ONE_MINUS_SRC_ALPHA);
}

static void M_SetBlendModeForUI(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

static void M_RenderSourcePass(const M_PRIV *const p, const SCENE_PASS pass)
{
    OUTPUT_SHADER *const shader = Output_GetMeshShader();
    for (int32_t i = 0; i < p->sources->count; i++) {
        const SCENE_SOURCE *const source =
            *(SCENE_SOURCE **)Vector_Get(p->sources, i);
        if (source->is_dirty != nullptr && source->is_dirty(source, pass)) {
            ASSERT(source->render_pass != nullptr);
            Output_Shader_UploadTint(shader, (RGB_F) { 1.0f, 1.0f, 1.0f });
            Output_Shader_UploadWibbleEffect(shader, false);
            source->render_pass(source, pass);
        }
    }
}

static bool M_IsSourceDirty(const M_PRIV *const p, const SCENE_PASS pass)
{
    for (int32_t i = 0; i < p->sources->count; i++) {
        const SCENE_SOURCE *const source =
            *(SCENE_SOURCE **)Vector_Get(p->sources, i);
        if (source->is_dirty != nullptr && source->is_dirty(source, pass)) {
            return true;
        }
    }
    return false;
}

static bool M_IsAnySourceDirty(const M_PRIV *const p)
{
    for (SCENE_PASS pass = 0; pass < SCENE_PASS_COUNT; pass++) {
        if (M_IsSourceDirty(p, pass)) {
            return true;
        }
    }
    return false;
}

static void M_PrepareScene(const M_PRIV *const p)
{
    OUTPUT_SHADER *const shader = Output_GetMeshShader();

#ifndef __APPLE__
    glLineWidth(g_Config.rendering.wireframe_width);
    GFX_GL_CheckError();
#endif

    glBindSampler(0, p->sampler_id);
    glSamplerParameterf(
        p->sampler_id, GL_TEXTURE_MAX_ANISOTROPY_EXT,
        g_Config.rendering.anisotropy_filter);
}

static void M_RenderScenePasses(const M_PRIV *const p)
{
    if (!M_IsAnySourceDirty(p)) {
        return;
    }

    OUTPUT_SHADER *const shader = Output_GetMeshShader();
    const bool wireframe = g_Config.rendering.enable_wireframe;

    M_SetSamplerFilter(p->sampler_id, g_Config.rendering.texture_filter);

    Output_Shader_Bind(shader);
    Output_Shader_UploadCommonUniforms(shader);
    M_BindTextures(p);
    M_SetupShaderForScene(
        shader, g_Config.rendering.texture_filter,
        g_Config.rendering.enable_lighting);
    M_SetBlendModeForScene(wireframe);

    glDisable(GL_DEPTH_TEST);
    if (M_IsSourceDirty(p, SCENE_PASS_BACKGROUND)) {
        M_RenderSourcePass(p, SCENE_PASS_BACKGROUND);
    }

    Output_Shader_Bind(shader);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    if (M_IsSourceDirty(p, SCENE_PASS_SKYBOX)) {
        M_RenderSourcePass(p, SCENE_PASS_SKYBOX);
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);

    if (M_IsSourceDirty(p, SCENE_PASS_MESHES)
        || M_IsSourceDirty(p, SCENE_PASS_TRANSPARENT)) {
        glEnable(GL_CULL_FACE);
        M_RenderSourcePass(p, SCENE_PASS_MESHES);
        M_RenderSourcePass(p, SCENE_PASS_TRANSPARENT);
        glDisable(GL_CULL_FACE);
    }

    if (M_IsSourceDirty(p, SCENE_PASS_UI)) {
        M_SetSamplerFilter(p->sampler_id, g_Config.rendering.ui_filter);
        M_SetupShaderForUI(shader, g_Config.rendering.ui_filter);
        M_SetBlendModeForUI();

        glClear(GL_DEPTH_BUFFER_BIT);
        M_RenderSourcePass(p, SCENE_PASS_UI);
    }
}

void SceneCompositor_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->sources = Vector_Create(sizeof(SCENE_SOURCE *));
    glGenSamplers(1, &p->sampler_id);
    glSamplerParameteri(p->sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(p->sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(p->sampler_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(p->sampler_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GFX_GL_CheckError();
}

void SceneCompositor_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->sources != nullptr) {
        Vector_Free(p->sources);
        p->sources = nullptr;
    }
    if (p->sampler_id != 0) {
        glDeleteSamplers(1, &p->sampler_id);
    }
}

bool M_IsActive(void)
{
    return !Output_IsHeadless() || Shell_GetArgs()->debug_render_performance
        || GFX_Context_GetScheduledScreenshotPath() != nullptr;
}

void SceneCompositor_BeginScene(void)
{
    M_PRIV *const p = &m_Priv;
    if (!M_IsActive()) {
        return;
    }
    M_PrepareScene(p);
    M_PROCESS_SOURCES(p, render_begin);
}

void SceneCompositor_Flush(void)
{
    M_PRIV *const p = &m_Priv;
    if (!M_IsActive()) {
        return;
    }
    M_RenderScenePasses(p);
    M_PROCESS_SOURCES(p, render_end);
    M_PROCESS_SOURCES(p, render_begin);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void SceneCompositor_EndScene(void)
{
    M_PRIV *const p = &m_Priv;
    if (!M_IsActive()) {
        return;
    }
    M_RenderScenePasses(p);
    M_PROCESS_SOURCES(p, render_end);
}

void SceneCompositor_AnimateTextures(void)
{
    M_PRIV *const p = &m_Priv;
    M_PROCESS_SOURCES(p, animate_textures);
}

void SceneCompositor_AddSource(const SCENE_SOURCE *const source)
{
    M_PRIV *const p = &m_Priv;
    Vector_Add(p->sources, &source);
}
