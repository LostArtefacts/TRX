#include "gfx/gl/buffer.h"

#include <assert.h>

void GFX_GL_Buffer_Init(GFX_GL_Buffer *buf, GLenum target)
{
    assert(buf);
    buf->target = target;
    glGenBuffers(1, &buf->id);
}

void GFX_GL_Buffer_Close(GFX_GL_Buffer *buf)
{
    assert(buf);
    glDeleteBuffers(1, &buf->id);
}

void GFX_GL_Buffer_Bind(GFX_GL_Buffer *buf)
{
    assert(buf);
    glBindBuffer(buf->target, buf->id);
}

void GFX_GL_Buffer_Data(
    GFX_GL_Buffer *buf, GLsizei size, const void *data, GLenum usage)
{
    assert(buf);
    glBufferData(buf->target, size, data, usage);
}

void GFX_GL_Buffer_SubData(
    GFX_GL_Buffer *buf, GLsizei offset, GLsizei size, const void *data)
{
    assert(buf);
    glBufferSubData(buf->target, offset, size, data);
}

void *GFX_GL_Buffer_Map(GFX_GL_Buffer *buf, GLenum access)
{
    assert(buf);
    return glMapBuffer(buf->target, access);
}

void GFX_GL_Buffer_Unmap(GFX_GL_Buffer *buf)
{
    assert(buf);
    glUnmapBuffer(buf->target);
}

GLint GFX_GL_Buffer_Parameter(GFX_GL_Buffer *buf, GLenum pname)
{
    assert(buf);
    GLint params = 0;
    glGetBufferParameteriv(buf->target, pname, &params);
    return params;
}
