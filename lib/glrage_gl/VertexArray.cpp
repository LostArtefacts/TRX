#include "VertexArray.hpp"

namespace glrage {
namespace gl {

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &m_id);
}

VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &m_id);
}

void VertexArray::bind()
{
    glBindVertexArray(m_id);
}

void VertexArray::attribute(GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, GLsizei offset)
{
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(
        index, size, type, normalized, stride, reinterpret_cast<void*>(offset));
}

} // namespace gl
} // namespace glrage
