#pragma once

#include "glrage_gl/Object.hpp"
#include "glrage_gl/gl_core_3_3.h"

#include <string>

namespace glrage {
namespace gl {

class Shader : public Object {
public:
    Shader(GLenum shaderType);
    ~Shader();
    void bind();
    Shader &fromFile(const std::string &path);
    Shader &fromString(const std::string &program);
    std::string infoLog();
    bool compiled();
};

}
}
