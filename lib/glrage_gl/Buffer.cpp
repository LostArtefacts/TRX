#include "Buffer.hpp"

namespace glrage {
namespace gl {

Buffer::Buffer(GLenum target)
    : m_target(target)
{
    glGenBuffers(1, &m_id);
}

Buffer::~Buffer()
{
    glDeleteBuffers(1, &m_id);
}

void Buffer::bind()
{
    glBindBuffer(m_target, m_id);
}

void Buffer::data(GLsizei size, const void* data, GLenum usage)
{
    glBufferData(m_target, size, data, usage);
}

void Buffer::subData(GLsizei offset, GLsizei size, const void* data)
{
    glBufferSubData(m_target, offset, size, data);
}

void* Buffer::map(GLenum access)
{
    return glMapBuffer(m_target, access);
}

void Buffer::unmap()
{
    glUnmapBuffer(m_target);
}

GLint Buffer::parameter(GLenum pname)
{
    GLint params = 0;
    glGetBufferParameteriv(m_target, pname, &params);
    return params;
}

} // namespace gl
} // namespace glrage
