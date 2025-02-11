#pragma once

#include <GL/glew.h>

typedef struct {
    bool initialized;
    GLuint id;
    GLenum target;
} GFX_GL_TEXTURE;

GFX_GL_TEXTURE *GFX_GL_Texture_Create(GLenum target);
void GFX_GL_Texture_Free(GFX_GL_TEXTURE *texture);

void GFX_GL_Texture_Init(GFX_GL_TEXTURE *texture, GLenum target);
void GFX_GL_Texture_Close(GFX_GL_TEXTURE *texture);
void GFX_GL_Texture_Bind(GFX_GL_TEXTURE *texture);
void GFX_GL_Texture_Load(
    GFX_GL_TEXTURE *texture, const void *data, int width, int height,
    GLint internal_format, GLint format);
void GFX_GL_Texture_LoadFromBackBuffer(GFX_GL_TEXTURE *texture);
