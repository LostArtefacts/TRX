#include "glrage_gl/Program.hpp"

#include "glrage_gl/ProgramException.hpp"

extern "C" {
#include "filesystem.h"
#include "log.h"
#include "memory.h"
#include "game/shell.h"
}

namespace glrage {
namespace gl {

Program::Program()
{
    m_id = glCreateProgram();
    if (!m_id) {
        throw ProgramException("Can't create shader program");
    }
}

Program::~Program()
{
    if (m_id) {
        glDeleteProgram(m_id);
    }
}

void Program::bind()
{
    glUseProgram(m_id);
}

void Program::attachShader(GLenum type, const std::string &path)
{
    GLuint shader_id = glCreateShader(type);
    if (!shader_id) {
        Shell_ExitSystem("Failed to create shader");
    }

    char *program = NULL;
    if (!File_Load(path.c_str(), &program, NULL)) {
        Shell_ExitSystemFmt("Unable to find shader file: %s", path.c_str());
    }

    glShaderSource(shader_id, 1, &program, NULL);
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

    if (program) {
        Memory_Free(program);
    }
    glAttachShader(m_id, shader_id);
    glDeleteShader(shader_id);
}

void Program::link()
{
    // do the linking
    glLinkProgram(m_id);

    // check for linking errors
    GLint linkStatus;
    glGetProgramiv(m_id, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        std::string message = infoLog();
        if (message.empty()) {
            message = "Shader linking failed.";
        }
        throw ProgramException(message);
    }
}

void Program::fragmentData(const std::string &name)
{
    glBindFragDataLocation(m_id, 0, name.c_str());
}

GLint Program::attributeLocation(const std::string &name)
{
    auto it = m_attributeLocations.find(name);
    if (it != m_attributeLocations.end()) {
        return it->second;
    }

    GLint location = glGetAttribLocation(m_id, name.c_str());
    if (location == -1) {
        LOG_INFO("Shader attribute not found: %s", name.c_str());
    }

    m_attributeLocations[name] = location;

    return location;
}

GLint Program::uniformLocation(const std::string &name)
{
    auto it = m_uniformLocations.find(name);
    if (it != m_uniformLocations.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(m_id, name.c_str());
    if (location == -1) {
        LOG_INFO("Shader uniform not found: %s", name.c_str());
    }

    m_uniformLocations[name] = location;

    return location;
}

void Program::uniform3f(
    const std::string &name, GLfloat v0, GLfloat v1, GLfloat v2)
{
    GLint loc = uniformLocation(name);
    if (loc != -1) {
        glUniform3f(loc, v0, v1, v2);
    }
}

void Program::uniform4f(
    const std::string &name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    GLint loc = uniformLocation(name);
    if (loc != -1) {
        glUniform4f(loc, v0, v1, v2, v3);
    }
}

void Program::uniform1i(const std::string &name, GLint v0)
{
    GLint loc = uniformLocation(name);
    if (loc != -1) {
        glUniform1i(loc, v0);
    }
}

void Program::uniformMatrix4fv(
    const std::string &name, GLsizei count, GLboolean transpose,
    const GLfloat *value)
{
    GLint loc = uniformLocation(name);
    if (loc != -1) {
        glUniformMatrix4fv(loc, count, transpose, value);
    }
}

std::string Program::infoLog()
{
    GLint infoLogLength;
    std::string infoLogString;
    infoLogString.resize(4096);
    glGetProgramInfoLog(
        m_id, infoLogString.capacity(), &infoLogLength, &infoLogString[0]);
    infoLogString.resize(infoLogLength);
    return infoLogString;
}

}
}
