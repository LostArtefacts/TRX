#include <trx/game/output/shaders/mesh.h>

#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/game/output/lights/priv.h>
#include <trx/game/output/state.h>
#include <trx/game/output/textures.h>
#include <trx/game/output/utils.h>
#include <trx/gl/utils.h>
#include <trx/version.h>

#include <string.h>

#define M_VARIANT_COUNT 6

// The lighting family picks the file; affine mapping picks whether the file
// gets its geometry stage, which splits near faces so the mapping has a
// smaller depth range to go wrong over. A game that maps textures with
// perspective needs no such split, so it binds a program with no geometry
// stage at all and pays nothing for the feature.
#define M_LIGHTING_VARIANT_COUNT 3

struct OUTPUT_MESH_SHADER {
    OUTPUT_SHADER *base[M_VARIANT_COUNT];

    MATRIX model_matrix[M_VARIANT_COUNT];
    bool has_model_matrix[M_VARIANT_COUNT];
    int32_t water_effect[M_VARIANT_COUNT];
    float water_effect_params[M_VARIANT_COUNT][3];
    bool is_wibble_effect[M_VARIANT_COUNT];
    bool is_alpha_discard_enabled[M_VARIANT_COUNT];
    RGBA_F tint[M_VARIANT_COUNT];
    OUTPUT_ATLAS_RECT env_map_rect[M_VARIANT_COUNT];
};

static const char *const m_VariantPaths[M_VARIANT_COUNT] = {
    "meshes_tr12.glsl", "meshes_tr3.glsl", "meshes_tr4.glsl",
    "meshes_tr12.glsl", "meshes_tr3.glsl", "meshes_tr4.glsl",
};

static bool M_VariantSubdivides(const int32_t variant_idx)
{
    return variant_idx >= M_LIGHTING_VARIANT_COUNT;
}

static int32_t M_GetVariantIndex(void)
{
    const int32_t lighting = Output_Lights_GetModel()->shader_variant;
    return g_Config.rendering.enable_affine_mapping
        ? lighting + M_LIGHTING_VARIANT_COUNT
        : lighting;
}

static OUTPUT_SHADER *M_GetVariantBase(
    const OUTPUT_MESH_SHADER *const shader, const int32_t variant_idx)
{
    return shader->base[variant_idx];
}

// TR4 samples the env map out of the atlas, so the variant needs to know which
// patch of it to use. The rect only changes on level load; the TR1-3 variants
// don't declare the uniforms at all.
static void M_UploadEnvMapRect(
    OUTPUT_MESH_SHADER *const shader, const int32_t variant_idx)
{
    const OUTPUT_ATLAS_RECT rect = Output_Textures_GetEnvMapRect();
    if (memcmp(&shader->env_map_rect[variant_idx], &rect, sizeof(rect)) == 0) {
        return;
    }
    shader->env_map_rect[variant_idx] = rect;

    OUTPUT_SHADER *const base = M_GetVariantBase(shader, variant_idx);
    GLint loc = -1;
    if (Output_Shader_TryLookupUniform(base, "uEnvMapUV0", &loc)) {
        TRX_GL_TRACK_UNIFORM(glUniform2f, loc, rect.uv0[0], rect.uv0[1]);
    }
    if (Output_Shader_TryLookupUniform(base, "uEnvMapUV1", &loc)) {
        TRX_GL_TRACK_UNIFORM(glUniform2f, loc, rect.uv1[0], rect.uv1[1]);
    }
    if (Output_Shader_TryLookupUniform(base, "uEnvMapLayer", &loc)) {
        TRX_GL_TRACK_UNIFORM(glUniform1i, loc, rect.layer);
    }
}

RESULT Output_MeshShader_Create(OUTPUT_MESH_SHADER **const out_shader)
{
    *out_shader = nullptr;
    OUTPUT_MESH_SHADER *const shader = Memory_Alloc(sizeof(*shader));
    for (int32_t i = 0; i < M_VARIANT_COUNT; i++) {
        shader->has_model_matrix[i] = false;
        shader->water_effect[i] = -1;
        shader->water_effect_params[i][0] = 0.0f;
        shader->water_effect_params[i][1] = 0.0f;
        shader->water_effect_params[i][2] = 0.0f;
        shader->is_wibble_effect[i] = false;
        shader->is_alpha_discard_enabled[i] = false;
        shader->tint[i] = (RGBA_F) { 0.0f, 0.0f, 0.0f, 0.0f };
        shader->env_map_rect[i] = (OUTPUT_ATLAS_RECT) { .layer = -1 };

        const bool subdivides = M_VariantSubdivides(i);
        MUST(Output_Shader_CreateEx(
            m_VariantPaths[i], subdivides ? "#define SUBDIVIDE\n" : nullptr,
            subdivides, &shader->base[i]));
        Output_Shader_Bind(shader->base[i]);
        TRX_GL_TRACK_UNIFORM(
            glUniform1i,
            Output_Shader_LookupUniform(shader->base[i], "uTexAtlas"), 0);
        // TR4 reflections sample the atlas, so only the TR1-3 variants declare
        // the framebuffer-capture env map.
        GLint loc = -1;
        if (Output_Shader_TryLookupUniform(
                shader->base[i], "uTexEnvMap", &loc)) {
            TRX_GL_TRACK_UNIFORM(glUniform1i, loc, 1);
        }
    }
    *out_shader = shader;
    return OK;
}

void Output_MeshShader_Bind(OUTPUT_MESH_SHADER *const shader)
{
    const int32_t variant_idx = M_GetVariantIndex();
    Output_Shader_Bind(M_GetVariantBase(shader, variant_idx));
    M_UploadEnvMapRect(shader, variant_idx);
}

void Output_MeshShader_Free(OUTPUT_MESH_SHADER *const shader)
{
    for (int32_t i = 0; i < M_VARIANT_COUNT; i++) {
        Output_Shader_Free(shader->base[i]);
    }
    Memory_Free(shader);
}

void Output_MeshShader_UploadModelMatrix(
    OUTPUT_MESH_SHADER *const shader, const MATRIX *const source)
{
    const int32_t variant_idx = M_GetVariantIndex();
    OUTPUT_SHADER *const base = M_GetVariantBase(shader, variant_idx);
    if (shader->has_model_matrix[variant_idx]
        && memcmp(&shader->model_matrix[variant_idx], source, sizeof(*source))
            == 0) {
        return;
    }
    memcpy(&shader->model_matrix[variant_idx], source, sizeof(*source));
    shader->has_model_matrix[variant_idx] = true;

    GLfloat m[4][4];
    Output_FillMatrix(m, source);

    TRX_GL_TRACK_UNIFORM(
        glUniformMatrix4fv, Output_Shader_LookupUniform(base, "uMatModel"), 1,
        GL_FALSE, &m[0][0]);
}

void Output_MeshShader_UploadAlphaDiscard(
    OUTPUT_MESH_SHADER *const shader, const bool is_enabled)
{
    const int32_t variant_idx = M_GetVariantIndex();
    OUTPUT_SHADER *const base = M_GetVariantBase(shader, variant_idx);
    if (is_enabled == shader->is_alpha_discard_enabled[variant_idx]) {
        return;
    }
    TRX_GL_TRACK_UNIFORM(
        glUniform1i, Output_Shader_LookupUniform(base, "uDiscardAlpha"),
        is_enabled);
    shader->is_alpha_discard_enabled[variant_idx] = is_enabled;
}

void Output_MeshShader_UploadWaterEffect(
    OUTPUT_MESH_SHADER *const shader, const int32_t water_effect)
{
    const int32_t variant_idx = M_GetVariantIndex();
    OUTPUT_SHADER *const base = M_GetVariantBase(shader, variant_idx);
    if (water_effect == shader->water_effect[variant_idx]) {
        return;
    }

    static const float m_ChoppyAmp[22] = {
        16.0f, 0.0f,   0.0f,   0.0f,   0.0f,   16.0f, 16.0f, 16.0f,
        16.0f, 53.0f,  53.0f,  53.0f,  53.0f,  90.0f, 90.0f, 90.0f,
        90.0f, 127.0f, 127.0f, 127.0f, 127.0f, 0.0f,
    };
    static const float m_ShimmerAmp[22] = {
        7.875f,   4.0f,     8.0f,     12.0f,    15.875f,  -3.875f,
        -7.875f,  -11.875f, -15.875f, -3.875f,  -7.875f,  -11.875f,
        -15.875f, -3.875f,  -7.875f,  -11.875f, -15.875f, -3.875f,
        -7.875f,  -11.875f, -15.875f, 0.0f,
    };
    static const float m_AbsIntensity[22] = {
        0.0f,  253.0f, 0.0f, 4.0f,  8.0f,  4.0f, 8.0f, 12.0f,
        16.0f, 4.0f,   8.0f, 12.0f, 16.0f, 4.0f, 8.0f, 12.0f,
        16.0f, 4.0f,   8.0f, 12.0f, 16.0f, 0.0f,
    };

    int32_t scheme = water_effect - 2;
    CLAMP(scheme, 0, 21);
    const float p0 = m_ChoppyAmp[scheme];
    const float p1 = m_ShimmerAmp[scheme];
    const float p2 = m_AbsIntensity[scheme];

    GLint loc = -1;
    if (Output_Shader_TryLookupUniform(base, "uWaterEffect", &loc)) {
        TRX_GL_TRACK_UNIFORM(glUniform1i, loc, water_effect);
    }
    if (Output_Shader_TryLookupUniform(base, "uWaterEffectParams", &loc)) {
        TRX_GL_TRACK_UNIFORM(glUniform3f, loc, p0, p1, p2);
    }
    shader->water_effect[variant_idx] = water_effect;
    shader->water_effect_params[variant_idx][0] = p0;
    shader->water_effect_params[variant_idx][1] = p1;
    shader->water_effect_params[variant_idx][2] = p2;
}

void Output_MeshShader_UploadWibbleEffect(
    OUTPUT_MESH_SHADER *const shader, const bool is_enabled)
{
    const int32_t variant_idx = M_GetVariantIndex();
    OUTPUT_SHADER *const base = M_GetVariantBase(shader, variant_idx);
    if (is_enabled == shader->is_wibble_effect[variant_idx]) {
        return;
    }
    TRX_GL_TRACK_UNIFORM(
        glUniform1i, Output_Shader_LookupUniform(base, "uWibbleEffect"),
        is_enabled);
    shader->is_wibble_effect[variant_idx] = is_enabled;
}

void Output_MeshShader_UploadTint(OUTPUT_MESH_SHADER *const shader, RGBA_F tint)
{
    const int32_t variant_idx = M_GetVariantIndex();
    OUTPUT_SHADER *const base = M_GetVariantBase(shader, variant_idx);
    if (tint.r == shader->tint[variant_idx].r
        && tint.g == shader->tint[variant_idx].g
        && tint.b == shader->tint[variant_idx].b
        && tint.a == shader->tint[variant_idx].a) {
        return;
    }
    TRX_GL_TRACK_UNIFORM(
        glUniform4f, Output_Shader_LookupUniform(base, "uTint"), tint.r, tint.g,
        tint.b, tint.a);
    shader->tint[variant_idx] = tint;
}
