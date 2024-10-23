#include "gfx/gl/buffer.h"

#include "gfx/gl/utils.h"

#include <assert.h>

void GFX_GL_Buffer_Init(GFX_GL_BUFFER *buf, GLenum target)
{
    assert(buf != NULL);
    buf->target = target;
    glGenBuffers(1, &buf->id);
    GFX_GL_CheckError();
    buf->initialized = true;
}

void GFX_GL_Buffer_Close(GFX_GL_BUFFER *buf)
{
    assert(buf != NULL);
    if (buf->initialized) {
        glDeleteBuffers(1, &buf->id);
        GFX_GL_CheckError();
    }
    buf->initialized = false;
}

void GFX_GL_Buffer_Bind(GFX_GL_BUFFER *buf)
{
    assert(buf != NULL);
    assert(buf->initialized);
    glBindBuffer(buf->target, buf->id);
    GFX_GL_CheckError();
}

void GFX_GL_Buffer_Data(
    GFX_GL_BUFFER *buf, GLsizei size, const void *data, GLenum usage)
{
    assert(buf != NULL);
    assert(buf->initialized);
    glBufferData(buf->target, size, data, usage);
    GFX_GL_CheckError();
}

void GFX_GL_Buffer_SubData(
    GFX_GL_BUFFER *buf, GLsizei offset, GLsizei size, const void *data)
{
    assert(buf != NULL);
    assert(buf->initialized);
    glBufferSubData(buf->target, offset, size, data);
    GFX_GL_CheckError();
}

void *GFX_GL_Buffer_Map(GFX_GL_BUFFER *buf, GLenum access)
{
    assert(buf != NULL);
    assert(buf->initialized);
    void *ret = glMapBuffer(buf->target, access);
    GFX_GL_CheckError();
    return ret;
}

void GFX_GL_Buffer_Unmap(GFX_GL_BUFFER *buf)
{
    assert(buf != NULL);
    assert(buf->initialized);
    glUnmapBuffer(buf->target);
    GFX_GL_CheckError();
}

GLint GFX_GL_Buffer_Parameter(GFX_GL_BUFFER *buf, GLenum pname)
{
    assert(buf != NULL);
    assert(buf->initialized);
    GLint params = 0;
    glGetBufferParameteriv(buf->target, pname, &params);
    GFX_GL_CheckError();
    return params;
}
