#pragma once

#include "Object.hpp"
#include "gl_core_3_3.h"

#include <string>

namespace glrage {
namespace gl {

class Shader : public Object
{
public:
    Shader(GLenum shaderType);
    ~Shader();
    void bind();
    Shader& fromFile(const std::wstring& path);
    Shader& fromString(const std::string& program);
    std::string infoLog();
    bool compiled();
};

} // namespace gl
} // namespace glrage