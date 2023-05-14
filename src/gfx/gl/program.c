#include "gfx/gl/program.h"

#include "filesystem.h"
#include "game/shell.h"
#include "gfx/gl/utils.h"
#include "log.h"
#include "memory.h"

#include <assert.h>
#include <stddef.h>

bool GFX_GL_Program_Init(GFX_GL_Program *program)
{
    assert(program);
    program->id = glCreateProgram();
    GFX_GL_CheckError();
    if (!program->id) {
        LOG_ERROR("Can't create shader program");
        return false;
    }
    return true;
}

void GFX_GL_Program_Close(GFX_GL_Program *program)
{
    if (program->id) {
        glDeleteProgram(program->id);
        GFX_GL_CheckError();
        program->id = 0;
    }
}

void GFX_GL_Program_Bind(GFX_GL_Program *program)
{
    glUseProgram(program->id);
    GFX_GL_CheckError();
}

void GFX_GL_Program_AttachShader(
    GFX_GL_Program *program, GLenum type, const char *path)
{
    GLuint shader_id = glCreateShader(type);
    GFX_GL_CheckError();
    if (!shader_id) {
        Shell_ExitSystem("Failed to create shader");
    }

    char *content = NULL;
    if (!File_Load(path, &content, NULL)) {
        Shell_ExitSystemFmt("Unable to find shader file: %s", path);
    }

    glShaderSource(shader_id, 1, (const char **)&content, NULL);

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

    Memory_FreePointer(&content);

    glAttachShader(program->id, shader_id);
    GFX_GL_CheckError();

    glDeleteShader(shader_id);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Link(GFX_GL_Program *program)
{
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

void GFX_GL_Program_FragmentData(GFX_GL_Program *program, const char *name)
{
    glBindFragDataLocation(program->id, 0, name);
    GFX_GL_CheckError();
}

GLint GFX_GL_Program_UniformLocation(GFX_GL_Program *program, const char *name)
{
    GLint location = glGetUniformLocation(program->id, name);
    GFX_GL_CheckError();
    if (location == -1) {
        LOG_INFO("Shader uniform not found: %s", name);
    }
    return location;
}

void GFX_GL_Program_Uniform3f(
    GFX_GL_Program *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2)
{
    glUniform3f(loc, v0, v1, v2);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Uniform4f(
    GFX_GL_Program *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2,
    GLfloat v3)
{
    glUniform4f(loc, v0, v1, v2, v3);
    GFX_GL_CheckError();
}

void GFX_GL_Program_Uniform1i(GFX_GL_Program *program, GLint loc, GLint v0)
{
    glUniform1i(loc, v0);
    GFX_GL_CheckError();
}

void GFX_GL_Program_UniformMatrix4fv(
    GFX_GL_Program *program, GLint loc, GLsizei count, GLboolean transpose,
    const GLfloat *value)
{
    glUniformMatrix4fv(loc, count, transpose, value);
    GFX_GL_CheckError();
}
