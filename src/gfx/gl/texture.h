#pragma once

#include "gfx/gl/gl_core_3_3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GFX_GL_Texture {
    GLuint id;
    GLenum target;
} GFX_GL_Texture;

void GFX_GL_Texture_Init(GFX_GL_Texture *texture, GLenum target);
void GFX_GL_Texture_Close(GFX_GL_Texture *texture);
void GFX_GL_Texture_Bind(GFX_GL_Texture *texture);
GLenum GFX_GL_Texture_Target(GFX_GL_Texture *texture);

#ifdef __cplusplus
}
#endif
