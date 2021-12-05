#include "glrage_gl/program.h"

#include "filesystem.h"
#include "log.h"
#include "memory.h"
#include "game/shell.h"

#include <assert.h>

bool GLRage_GLProgram_Init(GLRage_GLProgram *program)
{
    assert(program);
    program->id = glCreateProgram();
    if (!program->id) {
        LOG_ERROR("Can't create shader program");
        return false;
    }
    return true;
}

void GLRage_GLProgram_Close(GLRage_GLProgram *program)
{
    if (program->id) {
        glDeleteProgram(program->id);
        program->id = 0;
    }
}

void GLRage_GLProgram_Bind(GLRage_GLProgram *program)
{
    glUseProgram(program->id);
}

void GLRage_GLProgram_AttachShader(
    GLRage_GLProgram *program, GLenum type, const char *path)
{
    GLuint shader_id = glCreateShader(type);
    if (!shader_id) {
        Shell_ExitSystem("Failed to create shader");
    }

    char *content = NULL;
    if (!File_Load(path, &content, NULL)) {
        Shell_ExitSystemFmt("Unable to find shader file: %s", path);
    }

    glShaderSource(shader_id, 1, (const char **)&content, NULL);
    glCompileShader(shader_id);

    int compile_status;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
        GLsizei info_log_size = 4096;
        char info_log[info_log_size];
        glGetShaderInfoLog(shader_id, info_log_size, &info_log_size, info_log);
        if (info_log[0]) {
            Shell_ExitSystemFmt("Shader compilation failed:\n%s", info_log);
        } else {
            Shell_ExitSystemFmt("Shader compilation failed.");
        }
    }

    if (content) {
        Memory_Free(content);
    }
    glAttachShader(program->id, shader_id);
    glDeleteShader(shader_id);
}

void GLRage_GLProgram_Link(GLRage_GLProgram *program)
{
    glLinkProgram(program->id);

    GLint linkStatus;
    glGetProgramiv(program->id, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        GLsizei info_log_size = 4096;
        char info_log[info_log_size];
        glGetProgramInfoLog(
            program->id, info_log_size, &info_log_size, info_log);
        if (info_log[0]) {
            Shell_ExitSystemFmt("Shader linking failed:\n%s", info_log);
        } else {
            Shell_ExitSystemFmt("Shader linking failed.");
        }
    }
}

void GLRage_GLProgram_FragmentData(GLRage_GLProgram *program, const char *name)
{
    glBindFragDataLocation(program->id, 0, name);
}

GLint GLRage_GLProgram_UniformLocation(
    GLRage_GLProgram *program, const char *name)
{
    GLint location = glGetUniformLocation(program->id, name);
    if (location == -1) {
        LOG_INFO("Shader uniform not found: %s", name);
    }
    return location;
}

void GLRage_GLProgram_Uniform3f(
    GLRage_GLProgram *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2)
{
    glUniform3f(loc, v0, v1, v2);
}

void GLRage_GLProgram_Uniform4f(
    GLRage_GLProgram *program, GLint loc, GLfloat v0, GLfloat v1, GLfloat v2,
    GLfloat v3)
{
    glUniform4f(loc, v0, v1, v2, v3);
}

void GLRage_GLProgram_Uniform1i(GLRage_GLProgram *program, GLint loc, GLint v0)
{
    glUniform1i(loc, v0);
}

void GLRage_GLProgram_UniformMatrix4fv(
    GLRage_GLProgram *program, GLint loc, GLsizei count, GLboolean transpose,
    const GLfloat *value)
{
    glUniformMatrix4fv(loc, count, transpose, value);
}
