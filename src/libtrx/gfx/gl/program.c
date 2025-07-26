#include "gfx/gl/program.h"

#include "debug.h"
#include "filesystem.h"
#include "game/shell.h"
#include "gfx/gl/track.h"
#include "gfx/gl/utils.h"
#include "log.h"
#include "memory.h"

#include <string.h>

bool GFX_GL_Program_Init(GFX_GL_PROGRAM *program)
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

void GFX_GL_Program_Close(GFX_GL_PROGRAM *program)
{
    ASSERT(program != nullptr);
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

char *GFX_GL_Program_PreprocessShader(const char *content, GLenum type)
{
    ASSERT(content != nullptr);

    char *common = nullptr;
    File_Load("shaders/common.glsl", &common, nullptr);

    const char *version_ogl33c = "#version 330 core\n";
    const char *define_vertex = "#define VERTEX\n";
    const char *define_fragment = "#define FRAGMENT\n";
    const char *define_ogl33c = "#define OGL33C\n";

    size_t bufsize = strlen(content) + 1;
    if (common != nullptr) {
        bufsize += strlen(common);
    }

    bufsize += strlen(version_ogl33c);
    bufsize += strlen(define_ogl33c);

    if (type == GL_VERTEX_SHADER) {
        bufsize += strlen(define_vertex);
    } else if (type == GL_FRAGMENT_SHADER) {
        bufsize += strlen(define_fragment);
    }

    char *processed_content = Memory_Alloc(bufsize);
    strcpy(processed_content, version_ogl33c);
    strcat(processed_content, define_ogl33c);

    if (common != nullptr) {
        strcat(processed_content, common);
    }

    if (type == GL_VERTEX_SHADER) {
        strcat(processed_content, define_vertex);
    } else if (type == GL_FRAGMENT_SHADER) {
        strcat(processed_content, define_fragment);
    }

    strcat(processed_content, content);

    Memory_FreePointer(&common);
    return processed_content;
}

void GFX_GL_Program_AttachShader(
    GFX_GL_PROGRAM *program, GLenum type, const char *path)
{
    ASSERT(program != nullptr);
    ASSERT(path != nullptr);

    GLuint shader_id = glCreateShader(type);
    GFX_GL_CheckError();
    if (!shader_id) {
        Shell_ExitSystem("Failed to create shader");
    }

    char *content = nullptr;
    if (!File_Load(path, &content, nullptr)) {
        Shell_ExitSystemFmt("Unable to find shader file: %s", path);
    }

    char *processed_content = GFX_GL_Program_PreprocessShader(content, type);
    Memory_FreePointer(&content);
    if (!processed_content) {
        Shell_ExitSystemFmt("Failed to pre-process shader source:  %s", path);
    }

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
            Shell_ExitSystemFmt("Shader compilation failed:\n%s", info_log);
        } else {
            Shell_ExitSystemFmt("Shader compilation failed.");
        }
    }

    Memory_FreePointer(&processed_content);

    glAttachShader(program->id, shader_id);
    GFX_GL_CheckError();

    glDeleteShader(shader_id);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Link(GFX_GL_PROGRAM *program)
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
            Shell_ExitSystemFmt("Shader linking failed:\n%s", info_log);
        } else {
            Shell_ExitSystemFmt("Shader linking failed.");
        }
    }
}

void GFX_GL_Program_FragmentData(GFX_GL_PROGRAM *program, const char *name)
{
    ASSERT(program != nullptr);
    glBindFragDataLocation(program->id, 0, name);
    GFX_GL_CheckError();
}

GLint GFX_GL_Program_UniformLocation(GFX_GL_PROGRAM *program, const char *name)
{
    ASSERT(program != nullptr);
    GLint location = glGetUniformLocation(program->id, name);
    GFX_GL_CheckError();
    if (location == -1) {
        LOG_INFO("Shader uniform not found: %s", name);
    }
    return location;
}

void GFX_GL_Program_Uniform3f(
    GFX_GL_PROGRAM *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniform3f, loc, v0, v1, v2);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Uniform4f(
    GFX_GL_PROGRAM *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2,
    GLfloat v3)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniform4f, loc, v0, v1, v2, v3);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Uniform1i(GFX_GL_PROGRAM *program, GLint loc, GLint v0)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniform1i, loc, v0);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Uniform1f(GFX_GL_PROGRAM *program, GLint loc, GLfloat v0)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniform1f, loc, v0);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Uniform2f(
    GFX_GL_PROGRAM *program, GLint loc, GLfloat v0, GLfloat v1)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniform2f, loc, v0, v1);
    GFX_GL_CheckError();
}

void GFX_GL_Program_UniformMatrix4fv(
    GFX_GL_PROGRAM *program, GLint loc, GLsizei count, GLboolean transpose,
    const GLfloat *value)
{
    ASSERT(program != nullptr);
    GFX_TRACK_UNIFORM(glUniformMatrix4fv, loc, count, transpose, value);
    GFX_GL_CheckError();
}
