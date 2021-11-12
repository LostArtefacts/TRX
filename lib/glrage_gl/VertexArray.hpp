#pragma once

#include "Object.hpp"
#include "gl_core_3_3.h"

namespace glrage {
namespace gl {

class VertexArray : public Object
{
public:
    VertexArray();
    ~VertexArray();
    void bind();
    void attribute(GLuint index, GLint size, GLenum type, GLboolean normalized,
        GLsizei stride, GLsizei offset);
};

} // namespace gl
} // namespace glrage
