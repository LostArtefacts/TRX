#pragma once

#include "glrage_gl/Object.hpp"
#include "glrage_gl/gl_core_3_3.h"

namespace glrage {
namespace gl {

class Buffer : public Object {
public:
    Buffer(GLenum target);
    ~Buffer();
    void bind();
    void data(GLsizei size, const void *data, GLenum usage);
    void subData(GLsizei offset, GLsizei size, const void *data);
    void *map(GLenum access);
    void unmap();
    GLint parameter(GLenum pname);

private:
    GLenum m_target;
};

}
}
