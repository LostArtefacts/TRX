#pragma once

#include <GL/glew.h>

typedef struct {
    bool initialized;
    GLuint id;
    GLenum target;
} GFX_GL_BUFFER;

void GFX_GL_Buffer_Init(GFX_GL_BUFFER *buf, GLenum target);
void GFX_GL_Buffer_Close(GFX_GL_BUFFER *buf);

void GFX_GL_Buffer_Bind(GFX_GL_BUFFER *buf);
void GFX_GL_Buffer_Data(
    GFX_GL_BUFFER *buf, GLsizei size, const void *data, GLenum usage);
void GFX_GL_Buffer_SubData(
    GFX_GL_BUFFER *buf, GLsizei offset, GLsizei size, const void *data);
void *GFX_GL_Buffer_Map(GFX_GL_BUFFER *buf, GLenum access);
void GFX_GL_Buffer_Unmap(GFX_GL_BUFFER *buf);
GLint GFX_GL_Buffer_Parameter(GFX_GL_BUFFER *buf, GLenum pname);
