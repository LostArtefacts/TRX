#include <trx/game/output/shader.h>

#include <trx/debug.h>
#include <trx/game/output.h>
#include <trx/game/output/utils.h>
#include <trx/game/viewport.h>
#include <trx/gfx/gl/program.h>
#include <trx/gfx/gl/utils.h>
#include <trx/memory.h>

#include <uthash.h>

typedef struct {
    GLint location;
    GLenum type;
    char name[64];
    UT_hash_handle hh;
} M_UNIFORM;

struct OUTPUT_SHADER {
    GFX_GL_PROGRAM program;

    int32_t count;
    M_UNIFORM *uniforms;
    M_UNIFORM *uniform_hash;

    bool is_wibble_effect;
    bool is_alpha_discard_enabled;
    RGB_F tint;
};

static const char *const m_UniformBlocks[] = {
    "Globals",
    "Matrices",
    nullptr,
};

static GLint M_Uniform_Lookup(
    const OUTPUT_SHADER *const shader, const char *const name)
{
    M_UNIFORM *uniform = nullptr;
    HASH_FIND_STR(shader->uniform_hash, name, uniform);
    if (uniform == nullptr) {
        LOG_ERROR("Uniform %s not found", name);
        return -1;
    }
    return uniform->location;
}

static void M_DebugUBO(const GLuint program_id, const GLuint block_idx)
{
    // Prints memory layout of the specific UBO in the GPU

    // Get the block name
    GLint name_len = 0;
    glGetActiveUniformBlockiv(
        program_id, block_idx, GL_UNIFORM_BLOCK_NAME_LENGTH, &name_len);
    char *const block_name = Memory_Alloc(name_len);
    glGetActiveUniformBlockName(
        program_id, block_idx, name_len, nullptr, block_name);

    // Get all uniforms within that block
    GLint uniform_count = 0;
    glGetActiveUniformBlockiv(
        program_id, block_idx, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
        &uniform_count);
    GLuint *const uniform_indices =
        Memory_Alloc(sizeof(GLuint) * uniform_count);
    glGetActiveUniformBlockiv(
        program_id, block_idx, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
        (GLint *)uniform_indices);

    // Query offsets
    GLint *const offsets = Memory_Alloc(sizeof(GLint) * uniform_count);
    glGetActiveUniformsiv(
        program_id, uniform_count, uniform_indices, GL_UNIFORM_OFFSET, offsets);

    // Print block name and all members
    LOG_DEBUG("Uniform Block %u: %s", block_idx, block_name);
    for (GLint i = 0; i < uniform_count; ++i) {
        char name[256];
        GLsizei length;
        glGetActiveUniformName(
            program_id, uniform_indices[i], sizeof(name), &length, name);
        LOG_DEBUG("  %s → offset %d", name, offsets[i]);
    }

    // Cleanup
    Memory_Free(offsets);
    Memory_Free(uniform_indices);
    Memory_Free(block_name);
}

OUTPUT_SHADER *Output_Shader_Create(const char *const path)
{
    OUTPUT_SHADER *const shader = Memory_Alloc(sizeof(OUTPUT_SHADER));

    GFX_GL_Program_Init(&shader->program);
    GFX_GL_Program_AttachShader(&shader->program, GL_VERTEX_SHADER, path);
    GFX_GL_Program_AttachShader(&shader->program, GL_FRAGMENT_SHADER, path);
    GFX_GL_Program_FragmentData(&shader->program, "outColor");
    GFX_GL_Program_Link(&shader->program);

#if 0
    M_DebugUBO(shader->program.id, 0);
#endif

    // Bind uniform blocks to UBO binding points
    const GLuint program_id = shader->program.id;
    for (int32_t i = 0; m_UniformBlocks[i] != nullptr; i++) {
        GLuint block_index =
            glGetUniformBlockIndex(program_id, m_UniformBlocks[i]);
        if (block_index != GL_INVALID_INDEX) {
            glUniformBlockBinding(program_id, block_index, i);
        }
    }

    GLint count;
    glGetProgramiv(shader->program.id, GL_ACTIVE_UNIFORMS, &count);
    shader->count = count;
    shader->uniforms = Memory_Alloc(sizeof(M_UNIFORM) * count);
    shader->uniform_hash = nullptr;

    for (GLint i = 0; i < count; i++) {
        M_UNIFORM *const uniform = &shader->uniforms[i];

        GLsizei len;
        GLchar name[64];
        glGetActiveUniform(
            shader->program.id, i, sizeof(name), &len, nullptr, &uniform->type,
            name);

        uniform->location = glGetUniformLocation(shader->program.id, name);
        strncpy(uniform->name, name, sizeof(uniform->name));
        HASH_ADD_STR(shader->uniform_hash, name, uniform);
    }

    GFX_GL_Program_Bind(&shader->program);
    GFX_TRACK_UNIFORM(glUniform1i, M_Uniform_Lookup(shader, "uTexAtlas"), 0);
    GFX_TRACK_UNIFORM(glUniform1i, M_Uniform_Lookup(shader, "uTexEnvMap"), 1);
    return shader;
}

void Output_Shader_Free(OUTPUT_SHADER *const shader)
{
    GFX_GL_Program_Close(&shader->program);
    M_UNIFORM *cur, *tmp;
    HASH_ITER(hh, shader->uniform_hash, cur, tmp)
    {
        HASH_DEL(shader->uniform_hash, cur);
    }
    Memory_Free(shader->uniforms);
    Memory_Free(shader);
}

void Output_Shader_Bind(const OUTPUT_SHADER *const shader)
{
    ASSERT(shader != nullptr);
    GFX_GL_Program_Bind(&shader->program);
    const OUTPUT_UNIFORMS *const uniforms = Output_GetUniforms();
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms->general);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uniforms->matrices);
}

void Output_Shader_UploadModelMatrix(
    const OUTPUT_SHADER *const shader, const MATRIX *const source)
{
    GLfloat m[4][4];
    Output_FillMatrix(m, source);

    GFX_TRACK_UNIFORM(
        glUniformMatrix4fv, M_Uniform_Lookup(shader, "uMatModel"), 1, GL_FALSE,
        &m[0][0]);
}

void Output_Shader_UploadAlphaDiscard(
    OUTPUT_SHADER *const shader, const bool is_enabled)
{
    if (is_enabled == shader->is_alpha_discard_enabled) {
        return;
    }
    GFX_TRACK_UNIFORM(
        glUniform1i, M_Uniform_Lookup(shader, "uDiscardAlpha"), is_enabled);
    shader->is_alpha_discard_enabled = is_enabled;
}

void Output_Shader_UploadWibbleEffect(
    OUTPUT_SHADER *const shader, const bool is_enabled)
{
    if (is_enabled == shader->is_wibble_effect) {
        return;
    }
    GFX_TRACK_UNIFORM(
        glUniform1i, M_Uniform_Lookup(shader, "uWibbleEffect"), is_enabled);
    shader->is_wibble_effect = is_enabled;
}

void Output_Shader_UploadTint(OUTPUT_SHADER *const shader, const RGB_F tint)
{
    if (tint.r == shader->tint.r && tint.g == shader->tint.g
        && tint.b == shader->tint.b) {
        return;
    }
    GFX_TRACK_UNIFORM(
        glUniform3f, M_Uniform_Lookup(shader, "uGlobalTint"), tint.r, tint.g,
        tint.b);
    shader->tint = tint;
}
