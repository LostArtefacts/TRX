#include <trx/game/output/shaders/mesh.h>

#include <trx/game/output/utils.h>
#include <trx/gfx/gl/utils.h>
#include <trx/memory.h>

struct OUTPUT_MESH_SHADER {
    OUTPUT_SHADER *base;
    bool is_wibble_effect;
    bool is_alpha_discard_enabled;
    RGB_F tint;
};

OUTPUT_MESH_SHADER *Output_MeshShader_Create(void)
{
    OUTPUT_MESH_SHADER *const shader = Memory_Alloc(sizeof(*shader));
    shader->base = Output_Shader_Create("shaders/meshes.glsl");
    GFX_TRACK_UNIFORM(
        glUniform1i, Output_Shader_LookupUniform(shader->base, "uTexAtlas"), 0);
    GFX_TRACK_UNIFORM(
        glUniform1i, Output_Shader_LookupUniform(shader->base, "uTexEnvMap"),
        1);
    return shader;
}

void Output_MeshShader_Bind(const OUTPUT_MESH_SHADER *const shader)
{
    Output_Shader_Bind(shader->base);
}

void Output_MeshShader_Free(OUTPUT_MESH_SHADER *const shader)
{
    Output_Shader_Free(shader->base);
    Memory_Free(shader);
}

void Output_MeshShader_UploadModelMatrix(
    const OUTPUT_MESH_SHADER *const shader, const MATRIX *const source)
{
    GLfloat m[4][4];
    Output_FillMatrix(m, source);

    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv,
        Output_Shader_LookupUniform(shader->base, "uMatModel"), 1, GL_FALSE,
        &m[0][0]);
}

void Output_MeshShader_UploadAlphaDiscard(
    OUTPUT_MESH_SHADER *const shader, const bool is_enabled)
{
    if (is_enabled == shader->is_alpha_discard_enabled) {
        return;
    }
    GFX_TRACK_UNIFORM(
        glUniform1i, Output_Shader_LookupUniform(shader->base, "uDiscardAlpha"),
        is_enabled);
    shader->is_alpha_discard_enabled = is_enabled;
}

void Output_MeshShader_UploadWibbleEffect(
    OUTPUT_MESH_SHADER *const shader, const bool is_enabled)
{
    if (is_enabled == shader->is_wibble_effect) {
        return;
    }
    GFX_TRACK_UNIFORM(
        glUniform1i, Output_Shader_LookupUniform(shader->base, "uWibbleEffect"),
        is_enabled);
    shader->is_wibble_effect = is_enabled;
}

void Output_MeshShader_UploadTint(
    OUTPUT_MESH_SHADER *const shader, const RGB_F tint)
{
    if (tint.r == shader->tint.r && tint.g == shader->tint.g
        && tint.b == shader->tint.b) {
        return;
    }
    GFX_TRACK_UNIFORM(
        glUniform3f, Output_Shader_LookupUniform(shader->base, "uGlobalTint"),
        tint.r, tint.g, tint.b);
    shader->tint = tint;
}
