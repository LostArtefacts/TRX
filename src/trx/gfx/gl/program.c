#include <trx/gfx/gl/program.h>

#include <trx/debug.h>
#include <trx/filesystem.h>
#include <trx/game/shell.h>
#include <trx/gfx/gl/track.h>
#include <trx/gfx/gl/utils.h>
#include <trx/log.h>
#include <trx/memory.h>

#include <stdio.h>
#include <string.h>

static char *M_PreprocessIncludes(const char *src, const char *dir)
{
    ASSERT(src != nullptr);
    ASSERT(dir != nullptr);

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
            Shell_ExitSystemFmt(
                "Malformed #include directive near: %.32s", include);
        }

        char filename[512];
        strncpy(filename, start_quote + 1, end_quote - start_quote - 1);
        filename[end_quote - start_quote - 1] = '\0';

        // Build relative path
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, filename);

        char *include_src = nullptr;
        if (!File_Load(full_path, &include_src, nullptr)) {
            Shell_ExitSystemFmt("Failed to include shader file: %s", full_path);
        }

        // Handle nested includes
        char *include_dir = File_GetParentDirectory(full_path);
        char *processed_include =
            M_PreprocessIncludes(include_src, include_dir ? include_dir : dir);
        Memory_FreePointer(&include_src);
        Memory_FreePointer(&include_dir);

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
    return result;
}

static char *M_Preprocess(const char *content, GLenum type)
{
    ASSERT(content != nullptr);

    const char *version_ogl33c = "#version 330 core\n";
    const char *define_vertex = "#define VERTEX\n";
    const char *define_fragment = "#define FRAGMENT\n";

    size_t bufsize = strlen(content) + 1;

    bufsize += strlen(version_ogl33c);

    if (type == GL_VERTEX_SHADER) {
        bufsize += strlen(define_vertex);
    } else if (type == GL_FRAGMENT_SHADER) {
        bufsize += strlen(define_fragment);
    }

    char *processed_content = Memory_Alloc(bufsize);
    strcpy(processed_content, version_ogl33c);

    if (type == GL_VERTEX_SHADER) {
        strcat(processed_content, define_vertex);
    } else if (type == GL_FRAGMENT_SHADER) {
        strcat(processed_content, define_fragment);
    }

    strcat(processed_content, content);
    return processed_content;
}

bool GFX_GL_Program_Init(GFX_GL_PROGRAM *const program)
{
    ASSERT(program != nullptr);
    program->id = glCreateProgram();
    GFX_GL_CheckError();
    if (!program->id) {
        LOG_ERROR("Can't create shader program");
        return false;
    }
    return true;
}

void GFX_GL_Program_Close(GFX_GL_PROGRAM *const program)
{
    ASSERT(program != nullptr);
    Memory_FreePointer(&program->path);
    if (program->id) {
        glDeleteProgram(program->id);
        GFX_GL_CheckError();
        program->id = 0;
    }
}

void GFX_GL_Program_Bind(const GFX_GL_PROGRAM *const program)
{
    ASSERT(program != nullptr);
    glUseProgram(program->id);
    GFX_GL_CheckError();
}

void GFX_GL_Program_AttachShader(
    GFX_GL_PROGRAM *program, GLenum type, const char *path)
{
    ASSERT(program != nullptr);
    ASSERT(path != nullptr);

    const char *const resolved_path =
        TRXPath_Resolve(TRX_DYNAMIC_PATH_SHADER_FILE, path);
    Memory_FreePointer(&program->path);
    program->path = Memory_DupStr(resolved_path);

    GLuint shader_id = glCreateShader(type);
    GFX_GL_CheckError();
    if (!shader_id) {
        Shell_ExitSystem("Failed to create shader");
    }

    char *content = nullptr;
    char *processed_content = nullptr;
    if (!File_Load(program->path, &content, nullptr)) {
        Shell_ExitSystemFmt("Unable to find shader file: %s", program->path);
    }

    char *shader_dir = File_GetParentDirectory(program->path);
    processed_content = M_PreprocessIncludes(content, shader_dir);
    ASSERT(processed_content != nullptr);
    Memory_FreePointer(&shader_dir);
    Memory_FreePointer(&content);

    content = processed_content;
    processed_content = M_Preprocess(content, type);
    ASSERT(processed_content != nullptr);
    Memory_FreePointer(&content);

    glShaderSource(
        shader_id, 1, (const char *const *)&processed_content, nullptr);

    GFX_GL_CheckError();
    glCompileShader(shader_id);
    GFX_GL_CheckError();

    int compile_status;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);
    GFX_GL_CheckError();

    if (compile_status != GL_TRUE) {
        GLsizei info_log_size = 4096;
        char info_log[info_log_size];
        glGetShaderInfoLog(shader_id, info_log_size, &info_log_size, info_log);
        GFX_GL_CheckError();

        if (info_log[0]) {
            Shell_ExitSystemFmt(
                "%s: compilation failed\n%s", program->path, info_log);
        } else {
            Shell_ExitSystemFmt("%s: compilation failed.", program->path);
        }
    }

    Memory_FreePointer(&processed_content);

    glAttachShader(program->id, shader_id);
    GFX_GL_CheckError();

    glDeleteShader(shader_id);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Link(GFX_GL_PROGRAM *const program)
{
    ASSERT(program != nullptr);
    glLinkProgram(program->id);
    GFX_GL_CheckError();

    GLint linkStatus;
    glGetProgramiv(program->id, GL_LINK_STATUS, &linkStatus);
    GFX_GL_CheckError();

    if (!linkStatus) {
        GLsizei info_log_size = 4096;
        char info_log[info_log_size];
        glGetProgramInfoLog(
            program->id, info_log_size, &info_log_size, info_log);
        GFX_GL_CheckError();
        if (info_log[0]) {
            Shell_ExitSystemFmt(
                "%s: shader linking failed\n%s", program->path, info_log);
        } else {
            Shell_ExitSystemFmt("%s: shader linking failed", program->path);
        }
    }
}

void GFX_GL_Program_FragmentData(
    GFX_GL_PROGRAM *const program, const char *const name)
{
    ASSERT(program != nullptr);
    glBindFragDataLocation(program->id, 0, name);
    GFX_GL_CheckError();
}

GLint GFX_GL_Program_UniformLocation(
    GFX_GL_PROGRAM *const program, const char *const name)
{
    ASSERT(program != nullptr);
    GLint location = glGetUniformLocation(program->id, name);
    GFX_GL_CheckError();
    if (location == -1) {
        LOG_INFO("%s: uniform not found (%s)", program->path, name);
    }
    return location;
}

void GFX_GL_Program_Uniform4f(
    GFX_GL_PROGRAM *const program, const GLint loc, const GLfloat v0,
    const GLfloat v1, const GLfloat v2, const GLfloat v3)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniform4f, loc, v0, v1, v2, v3);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Uniform1i(
    GFX_GL_PROGRAM *const program, const GLint loc, const GLint v0)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniform1i, loc, v0);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Uniform1f(
    GFX_GL_PROGRAM *const program, const GLint loc, const GLfloat v0)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniform1f, loc, v0);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Uniform2f(
    GFX_GL_PROGRAM *const program, const GLint loc, const GLfloat v0,
    const GLfloat v1)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniform2f, loc, v0, v1);
    GFX_GL_CheckError();
}
