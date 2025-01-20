#include "gfx/gl/buffer.h"

#include "debug.h"
#include "gfx/gl/utils.h"

void GFX_GL_Buffer_Init(GFX_GL_BUFFER *buf, GLenum target)
{
    ASSERT(buf != nullptr);
    buf->target = target;
    glGenBuffers(1, &buf->id);
    GFX_GL_CheckError();
    buf->initialized = true;
}

void GFX_GL_Buffer_Close(GFX_GL_BUFFER *buf)
{
    ASSERT(buf != nullptr);
    if (buf->initialized) {
        glDeleteBuffers(1, &buf->id);
        GFX_GL_CheckError();
    }
    buf->initialized = false;
}

void GFX_GL_Buffer_Bind(GFX_GL_BUFFER *buf)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    glBindBuffer(buf->target, buf->id);
    GFX_GL_CheckError();
}

void GFX_GL_Buffer_Data(
    GFX_GL_BUFFER *buf, GLsizei size, const void *data, GLenum usage)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    GFX_TRACK_DATA(glBufferData, buf->target, size, data, usage);
    GFX_GL_CheckError();
}

void GFX_GL_Buffer_SubData(
    GFX_GL_BUFFER *buf, GLsizei offset, GLsizei size, const void *data)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    GFX_TRACK_SUBDATA(glBufferSubData, buf->target, offset, size, data);
    GFX_GL_CheckError();
}

void *GFX_GL_Buffer_Map(GFX_GL_BUFFER *buf, GLenum access)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    void *ret = glMapBuffer(buf->target, access);
    GFX_GL_CheckError();
    return ret;
}

void GFX_GL_Buffer_Unmap(GFX_GL_BUFFER *buf)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    glUnmapBuffer(buf->target);
    GFX_GL_CheckError();
}

GLint GFX_GL_Buffer_Parameter(GFX_GL_BUFFER *buf, GLenum pname)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    GLint params = 0;
    glGetBufferParameteriv(buf->target, pname, &params);
    GFX_GL_CheckError();
    return params;
}
