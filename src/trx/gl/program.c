#include <trx/gl/program.h>

#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/shell.h>
#include <trx/gl/track.h>
#include <trx/gl/utils.h>

#include <stdio.h>
#include <string.h>

typedef struct {
    char *path;
    char *content;
} M_SHADER_FILE_CACHE_ENTRY;

static VECTOR *m_ShaderFileCache = nullptr; // M_SHADER_FILE_CACHE_ENTRY

static const char *M_LoadFileCached(const char *const path)
{
    ASSERT(path != nullptr);
    if (m_ShaderFileCache == nullptr) {
        m_ShaderFileCache = Vector_Create(sizeof(M_SHADER_FILE_CACHE_ENTRY));
    }
    for (int32_t i = 0; i < m_ShaderFileCache->count; i++) {
        M_SHADER_FILE_CACHE_ENTRY *const entry =
            Vector_Get(m_ShaderFileCache, i);
        if (strcmp(entry->path, path) == 0) {
            return entry->content;
        }
    }

    char *content = nullptr;
    if (!SHOULD(FS_Load(path, &content, nullptr))) {
        return nullptr;
    }
    M_SHADER_FILE_CACHE_ENTRY entry = {
        .path = Memory_DupStr(path),
        .content = content,
    };
    Vector_Add(m_ShaderFileCache, &entry);
    return entry.content;
}

__attribute__((destructor)) static void M_ShutdownCache(void)
{
    if (m_ShaderFileCache == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_ShaderFileCache->count; i++) {
        M_SHADER_FILE_CACHE_ENTRY *const entry =
            Vector_Get(m_ShaderFileCache, i);
        Memory_FreePointer(&entry->path);
        Memory_FreePointer(&entry->content);
    }
    Vector_Free(m_ShaderFileCache);
    m_ShaderFileCache = nullptr;
}

static RESULT M_PreprocessIncludes(
    const char *src, const char *dir, char **out_result)
{
    ASSERT(src != nullptr);
    ASSERT(dir != nullptr);
    *out_result = nullptr;

    const char *p = src;
    size_t result_cap = strlen(src) + 1;
    char *result = Memory_Alloc(result_cap);
    size_t used = 0;
    result[0] = '\0';

    while (*p != '\0') {
        const char *include = strstr(p, "#include");
        if (include == nullptr) {
            size_t tail_len = strlen(p);
            if (used + tail_len + 1 > result_cap) {
                result_cap = (used + tail_len + 1) * 2;
                result = Memory_Realloc(result, result_cap);
            }
            memcpy(result + used, p, tail_len);
            used += tail_len;
            result[used] = '\0';
            break;
        }

        // Copy text before #include
        size_t prefix_len = include - p;
        if (prefix_len > 0) {
            if (used + prefix_len + 1 > result_cap) {
                result_cap = (used + prefix_len + 1) * 2;
                result = Memory_Realloc(result, result_cap);
            }
            memcpy(result + used, p, prefix_len);
            used += prefix_len;
            result[used] = '\0';
        }

        // Parse filename between quotes
        const char *start_quote = strchr(include, '"');
        const char *end_quote =
            start_quote ? strchr(start_quote + 1, '"') : nullptr;
        if (!start_quote || !end_quote) {
            Memory_FreePointer(&result);
            return FAIL("a #include says no file name near: %.32s", include);
        }

        char filename[512];
        strncpy(filename, start_quote + 1, end_quote - start_quote - 1);
        filename[end_quote - start_quote - 1] = '\0';

        // Build relative path
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, filename);

        const char *const include_src = M_LoadFileCached(full_path);
        if (include_src == nullptr) {
            Memory_FreePointer(&result);
            return FAIL("%s: the included shader could not be read", full_path);
        }

        // Handle nested includes
        char *include_dir = FS_GetParentDirectory(full_path);
        char *processed_include = nullptr;
        const RESULT included = M_PreprocessIncludes(
            include_src, include_dir ? include_dir : dir, &processed_include);
        Memory_FreePointer(&include_dir);
        if (!IS_OK(included)) {
            Memory_FreePointer(&result);
            return included;
        }

        // Append included content
        size_t block_len = strlen(processed_include);
        if (used + block_len + 1 > result_cap) {
            result_cap = (used + block_len + 1) * 2;
            result = Memory_Realloc(result, result_cap);
        }
        memcpy(result + used, processed_include, block_len);
        used += block_len;
        result[used] = '\0';

        Memory_FreePointer(&processed_include);

        // Move past include line
        const char *next_line = strchr(end_quote, '\n');
        p = next_line ? next_line + 1 : end_quote + 1;
    }

    result[used] = '\0';
    *out_result = result;
    return OK;
}

static char *M_Preprocess(
    const char *const content, const GLenum type, const char *const defines)
{
    ASSERT(content != nullptr);

    const char *const version_ogl33c = "#version 330 core\n";
    const char *stage_define = "";
    if (type == GL_VERTEX_SHADER) {
        stage_define = "#define VERTEX\n";
    } else if (type == GL_GEOMETRY_SHADER) {
        stage_define = "#define GEOMETRY\n";
    } else if (type == GL_FRAGMENT_SHADER) {
        stage_define = "#define FRAGMENT\n";
    }

    const size_t bufsize = strlen(version_ogl33c) + strlen(stage_define)
        + (defines != nullptr ? strlen(defines) : 0) + strlen(content) + 1;

    char *const processed_content = Memory_Alloc(bufsize);
    strcpy(processed_content, version_ogl33c);
    strcat(processed_content, stage_define);
    if (defines != nullptr) {
        strcat(processed_content, defines);
    }
    strcat(processed_content, content);
    return processed_content;
}

RESULT TRX_GL_Program_Init(TRX_GL_PROGRAM *const program)
{
    ASSERT(program != nullptr);
    program->id = glCreateProgram();
    TRX_GL_CheckError();
    FAIL_IF(!program->id, "the shader program could not be created");
    return OK;
}

void TRX_GL_Program_Close(TRX_GL_PROGRAM *const program)
{
    ASSERT(program != nullptr);
    Memory_FreePointer(&program->path);
    if (program->id) {
        glDeleteProgram(program->id);
        TRX_GL_CheckError();
        program->id = 0;
    }
}

void TRX_GL_Program_Bind(const TRX_GL_PROGRAM *const program)
{
    ASSERT(program != nullptr);
    glUseProgram(program->id);
    TRX_GL_CheckError();
}

RESULT TRX_GL_Program_AttachShader(
    TRX_GL_PROGRAM *program, GLenum type, const char *path, const char *defines)
{
    ASSERT(program != nullptr);
    ASSERT(path != nullptr);

    const char *resolved_path = nullptr;
    MUST(GamePath_Resolve(GAME_DYNAMIC_PATH_SHADER_FILE, path, &resolved_path));
    Memory_FreePointer(&program->path);
    program->path = Memory_DupStr(resolved_path);

    GLuint shader_id = glCreateShader(type);
    TRX_GL_CheckError();
    FAIL_IF(!shader_id, "%s: the shader could not be created", program->path);

    const char *content = M_LoadFileCached(program->path);
    char *processed_content = nullptr;
    FAIL_IF(
        content == nullptr, "%s: the shader could not be read", program->path);

    char *shader_dir = FS_GetParentDirectory(program->path);
    const RESULT preprocessed =
        M_PreprocessIncludes(content, shader_dir, &processed_content);
    Memory_FreePointer(&shader_dir);
    MUST(preprocessed, "%s", program->path);
    ASSERT(processed_content != nullptr);

    char *expanded_content = processed_content;
    processed_content = M_Preprocess(expanded_content, type, defines);
    ASSERT(processed_content != nullptr);
    Memory_FreePointer(&expanded_content);

    glShaderSource(
        shader_id, 1, (const char *const *)&processed_content, nullptr);

    TRX_GL_CheckError();
    glCompileShader(shader_id);
    TRX_GL_CheckError();

    int compile_status;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);
    TRX_GL_CheckError();

    if (compile_status != GL_TRUE) {
        GLsizei info_log_size = 4096;
        char info_log[info_log_size];
        glGetShaderInfoLog(shader_id, info_log_size, &info_log_size, info_log);
        TRX_GL_CheckError();

        Memory_FreePointer(&processed_content);
        glDeleteShader(shader_id);
        if (info_log[0]) {
            return FAIL(
                "%s: the shader did not compile\n%s", program->path, info_log);
        }
        return FAIL("%s: the shader did not compile", program->path);
    }

    Memory_FreePointer(&processed_content);

    glAttachShader(program->id, shader_id);
    TRX_GL_CheckError();

    glDeleteShader(shader_id);
    TRX_GL_CheckError();
    return OK;
}

RESULT TRX_GL_Program_Link(TRX_GL_PROGRAM *const program)
{
    ASSERT(program != nullptr);
    glLinkProgram(program->id);
    TRX_GL_CheckError();

    GLint linkStatus;
    glGetProgramiv(program->id, GL_LINK_STATUS, &linkStatus);
    TRX_GL_CheckError();

    if (!linkStatus) {
        GLsizei info_log_size = 4096;
        char info_log[info_log_size];
        glGetProgramInfoLog(
            program->id, info_log_size, &info_log_size, info_log);
        TRX_GL_CheckError();
        if (info_log[0]) {
            return FAIL(
                "%s: the shader program did not link\n%s", program->path,
                info_log);
        }
        return FAIL("%s: the shader program did not link", program->path);
    }
    return OK;
}

void TRX_GL_Program_FragmentData(
    TRX_GL_PROGRAM *const program, const char *const name)
{
    ASSERT(program != nullptr);
    glBindFragDataLocation(program->id, 0, name);
    TRX_GL_CheckError();
}

GLint TRX_GL_Program_UniformLocation(
    TRX_GL_PROGRAM *const program, const char *const name)
{
    ASSERT(program != nullptr);
    GLint location = glGetUniformLocation(program->id, name);
    TRX_GL_CheckError();
    if (location == -1) {
        LOG_INFO("%s: uniform not found (%s)", program->path, name);
    }
    return location;
}

void TRX_GL_Program_Uniform4f(
    TRX_GL_PROGRAM *const program, const GLint loc, const GLfloat v0,
    const GLfloat v1, const GLfloat v2, const GLfloat v3)
{
    ASSERT(program != nullptr);
    TRX_GL_TRACK_UNIFORM(glUniform4f, loc, v0, v1, v2, v3);
    TRX_GL_CheckError();
}

void TRX_GL_Program_Uniform1i(
    TRX_GL_PROGRAM *const program, const GLint loc, const GLint v0)
{
    ASSERT(program != nullptr);
    TRX_GL_TRACK_UNIFORM(glUniform1i, loc, v0);
    TRX_GL_CheckError();
}

void TRX_GL_Program_Uniform1f(
    TRX_GL_PROGRAM *const program, const GLint loc, const GLfloat v0)
{
    ASSERT(program != nullptr);
    TRX_GL_TRACK_UNIFORM(glUniform1f, loc, v0);
    TRX_GL_CheckError();
}

void TRX_GL_Program_Uniform2f(
    TRX_GL_PROGRAM *const program, const GLint loc, const GLfloat v0,
    const GLfloat v1)
{
    ASSERT(program != nullptr);
    TRX_GL_TRACK_UNIFORM(glUniform2f, loc, v0, v1);
    TRX_GL_CheckError();
}
