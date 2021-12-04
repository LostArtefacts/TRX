#include "glrage_gl/buffer.h"

#include <assert.h>

void GLRage_GLBuffer_Init(GLRage_GLBuffer *buf, GLenum target)
{
    assert(buf);
    buf->target = target;
    glGenBuffers(1, &buf->id);
}

void GLRage_GLBuffer_Close(GLRage_GLBuffer *buf)
{
    assert(buf);
    glDeleteBuffers(1, &buf->id);
}

void GLRage_GLBuffer_Bind(GLRage_GLBuffer *buf)
{
    assert(buf);
    glBindBuffer(buf->target, buf->id);
}

void GLRage_GLBuffer_Data(
    GLRage_GLBuffer *buf, GLsizei size, const void *data, GLenum usage)
{
    assert(buf);
    glBufferData(buf->target, size, data, usage);
}

void GLRage_GLBuffer_SubData(
    GLRage_GLBuffer *buf, GLsizei offset, GLsizei size, const void *data)
{
    assert(buf);
    glBufferSubData(buf->target, offset, size, data);
}

void *GLRage_GLBuffer_Map(GLRage_GLBuffer *buf, GLenum access)
{
    assert(buf);
    return glMapBuffer(buf->target, access);
}

void GLRage_GLBuffer_Unmap(GLRage_GLBuffer *buf)
{
    assert(buf);
    glUnmapBuffer(buf->target);
}

GLint GLRage_GLBuffer_Parameter(GLRage_GLBuffer *buf, GLenum pname)
{
    assert(buf);
    GLint params = 0;
    glGetBufferParameteriv(buf->target, pname, &params);
    return params;
}
