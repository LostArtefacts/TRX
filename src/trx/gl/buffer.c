#include <trx/gl/buffer.h>

#include <trx/debug.h>
#include <trx/gl/track.h>
#include <trx/gl/utils.h>

void TRX_GL_Buffer_Init(TRX_GL_BUFFER *buf, GLenum target)
{
    ASSERT(buf != nullptr);
    buf->target = target;
    glGenBuffers(1, &buf->id);
    TRX_GL_CheckError();
    buf->initialized = true;
}

void TRX_GL_Buffer_Close(TRX_GL_BUFFER *buf)
{
    ASSERT(buf != nullptr);
    if (buf->initialized) {
        glDeleteBuffers(1, &buf->id);
        TRX_GL_CheckError();
    }
    buf->initialized = false;
}

void TRX_GL_Buffer_Bind(TRX_GL_BUFFER *buf)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    glBindBuffer(buf->target, buf->id);
    TRX_GL_CheckError();
}

void TRX_GL_Buffer_Data(
    TRX_GL_BUFFER *buf, GLsizei size, const void *data, GLenum usage)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    TRX_GL_TRACK_DATA(glBufferData, buf->target, size, data, usage);
    TRX_GL_CheckError();
}

void TRX_GL_Buffer_SubData(
    TRX_GL_BUFFER *buf, GLsizei offset, GLsizei size, const void *data)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    TRX_GL_TRACK_SUBDATA(glBufferSubData, buf->target, offset, size, data);
    TRX_GL_CheckError();
}

void *TRX_GL_Buffer_Map(TRX_GL_BUFFER *buf, GLenum access)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
#ifdef EMSCRIPTEN_BUILD
    // glMapBuffer is not available in GL ES 3.0; use glMapBufferRange instead.
    GLint size = 0;
    glGetBufferParameteriv(buf->target, GL_BUFFER_SIZE, &size);
    TRX_GL_CheckError();
    if (size <= 0) {
        return NULL;
    }
    GLbitfield flags = 0;
    if (access == GL_READ_ONLY) {
        flags = GL_MAP_READ_BIT;
    } else if (access == GL_WRITE_ONLY) {
        flags = GL_MAP_WRITE_BIT;
    } else {
        flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
    }
    void *ret = glMapBufferRange(buf->target, 0, size, flags);
#else
    void *ret = glMapBuffer(buf->target, access);
#endif
    TRX_GL_CheckError();
    return ret;
}

void TRX_GL_Buffer_Unmap(TRX_GL_BUFFER *buf)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    glUnmapBuffer(buf->target);
    TRX_GL_CheckError();
}

GLint TRX_GL_Buffer_Parameter(TRX_GL_BUFFER *buf, GLenum pname)
{
    ASSERT(buf != nullptr);
    ASSERT(buf->initialized);
    GLint params = 0;
    glGetBufferParameteriv(buf->target, pname, &params);
    TRX_GL_CheckError();
    return params;
}
