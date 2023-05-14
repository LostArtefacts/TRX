#include "gfx/gl/buffer.h"

#include "gfx/gl/utils.h"

#include <assert.h>

void GFX_GL_Buffer_Init(GFX_GL_Buffer *buf, GLenum target)
{
    assert(buf);
    buf->target = target;
    glGenBuffers(1, &buf->id);
    GFX_GL_CheckError();
}

void GFX_GL_Buffer_Close(GFX_GL_Buffer *buf)
{
    assert(buf);
    glDeleteBuffers(1, &buf->id);
    GFX_GL_CheckError();
}

void GFX_GL_Buffer_Bind(GFX_GL_Buffer *buf)
{
    assert(buf);
    glBindBuffer(buf->target, buf->id);
    GFX_GL_CheckError();
}

void GFX_GL_Buffer_Data(
    GFX_GL_Buffer *buf, GLsizei size, const void *data, GLenum usage)
{
    assert(buf);
    glBufferData(buf->target, size, data, usage);
    GFX_GL_CheckError();
}

void GFX_GL_Buffer_SubData(
    GFX_GL_Buffer *buf, GLsizei offset, GLsizei size, const void *data)
{
    assert(buf);
    glBufferSubData(buf->target, offset, size, data);
    GFX_GL_CheckError();
}

void *GFX_GL_Buffer_Map(GFX_GL_Buffer *buf, GLenum access)
{
    assert(buf);
    void *ret = glMapBuffer(buf->target, access);
    GFX_GL_CheckError();
    return ret;
}

void GFX_GL_Buffer_Unmap(GFX_GL_Buffer *buf)
{
    assert(buf);
    glUnmapBuffer(buf->target);
    GFX_GL_CheckError();
}

GLint GFX_GL_Buffer_Parameter(GFX_GL_Buffer *buf, GLenum pname)
{
    assert(buf);
    GLint params = 0;
    glGetBufferParameteriv(buf->target, pname, &params);
    GFX_GL_CheckError();
    return params;
}
