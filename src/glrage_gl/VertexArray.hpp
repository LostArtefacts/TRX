#pragma once

#include "glrage_gl/Object.hpp"
#include "glrage_gl/gl_core_3_3.h"

namespace glrage {
namespace gl {

class VertexArray : public Object {
public:
    VertexArray();
    ~VertexArray();
    void bind();
    void attribute(
        GLuint index, GLint size, GLenum type, GLboolean normalized,
        GLsizei stride, GLsizei offset);
};

}
}
