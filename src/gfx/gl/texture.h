#pragma once

#include "gfx/gl/gl_core_3_3.h"

typedef struct GFX_GL_Texture {
    GLuint id;
    GLenum target;
} GFX_GL_Texture;

GFX_GL_Texture *GFX_GL_Texture_Create(GLenum target);
void GFX_GL_Texture_Free(GFX_GL_Texture *texture);

void GFX_GL_Texture_Init(GFX_GL_Texture *texture, GLenum target);
void GFX_GL_Texture_Close(GFX_GL_Texture *texture);
void GFX_GL_Texture_Bind(GFX_GL_Texture *texture);
void GFX_GL_Texture_Load(
    GFX_GL_Texture *texture, const void *data, int width, int height,
    GLint internal_format, GLint format);
