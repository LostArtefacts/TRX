// Framebuffer object abstraction for off-screen rendering.

#pragma once

#include "game/viewport.h"
#include "gfx/context.h"
#include "gfx/gl/texture.h"

#include <GL/glew.h>

// Off-screen framebuffer with a single color attachment and optional
// depth+stencil.
typedef struct {
    int32_t width;
    int32_t height;
    GLuint fbo;
    GLuint rbo;
    GLint internal_format;
    GLenum format;
    bool with_depth_stencil;
    GFX_GL_TEXTURE texture;
} GFX_GL_FBO;

// Initialize an off-screen FBO.
void GFX_GL_FBO_Init(
    GFX_GL_FBO *fbo, int32_t width, int32_t height, GLint internal_format,
    GLenum format, bool with_depth_stencil);

// Close and free the GL resources.
void GFX_GL_FBO_Close(GFX_GL_FBO *fbo);

// Resize the FBO attachments (reallocate textures and RBO).
void GFX_GL_FBO_ResizeIfNeeded(GFX_GL_FBO *fbo, int32_t width, int32_t height);

// Bind this FBO for rendering (GL_FRAMEBUFFER).
void GFX_GL_FBO_Bind(const GFX_GL_FBO *fbo);

// Bind the default framebuffer (0).
void GFX_GL_FBO_Unbind(void);
