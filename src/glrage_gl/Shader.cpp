#include "glrage_gl/Shader.hpp"

#include "glrage_gl/ShaderException.hpp"
#include "glrage_util/ErrorUtils.hpp"
#include "glrage_util/StringUtils.hpp"

#include <string>

namespace glrage {
namespace gl {

Shader::Shader(GLenum type)
{
    m_id = glCreateShader(type);
    if (!m_id) {
        throw ShaderException("Can't create shader");
    }
}

Shader::~Shader()
{
    if (m_id) {
        glDeleteShader(m_id);
    }
}

void Shader::bind()
{
}

Shader &Shader::fromFile(const std::string &path)
{
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) {
        throw std::runtime_error(
            "Can't open shader file '" + path
            + "': " + ErrorUtils::getSystemErrorString());
    }

    fseek(fp, 0, SEEK_END);
    const auto size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::string content(size + 1, '\0');
    fread(&content[0], 1, size, fp);
    fclose(fp);

    fromString(content.c_str());

    return *this;
}

Shader &Shader::fromString(const std::string &program)
{
    // create shader source
    const char *programChars = program.c_str();
    glShaderSource(m_id, 1, &programChars, nullptr);

    // compile the shader
    glCompileShader(m_id);

    // check the compilation status and throw exception if shader
    // compilation failed
    if (!compiled()) {
        std::string message = infoLog();
        if (message.empty()) {
            message = "Shader compilation failed.";
        }
        throw ShaderException(message);
    }

    return *this;
}

std::string Shader::infoLog()
{
    GLint infoLogLength;
    std::string infoLogString;
    infoLogString.resize(4096);
    glGetShaderInfoLog(
        m_id, infoLogString.size(), &infoLogLength, &infoLogString[0]);
    infoLogString.resize(infoLogLength);
    return infoLogString;
}

bool Shader::compiled()
{
    int compileStatus;
    glGetShaderiv(m_id, GL_COMPILE_STATUS, &compileStatus);
    return compileStatus == GL_TRUE;
}

}
}
