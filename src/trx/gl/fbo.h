// Framebuffer object abstraction for off-screen rendering.

#pragma once

#include <trx/game/viewport.h>
#include <trx/gl/context.h>
#include <trx/gl/texture.h>

#ifdef EMSCRIPTEN_BUILD
    #include <trx/gl/gl_webgl_compat.h>
#else
    #include <GL/glew.h>
#endif

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
    TRX_GL_TEXTURE texture;
} TRX_GL_FBO;

// Initialize an off-screen FBO.
void TRX_GL_FBO_Init(
    TRX_GL_FBO *fbo, int32_t width, int32_t height, GLint internal_format,
    GLenum format, bool with_depth_stencil);

// Close and free the GL resources.
void TRX_GL_FBO_Close(TRX_GL_FBO *fbo);

// Resize the FBO attachments (reallocate textures and RBO).
void TRX_GL_FBO_ResizeIfNeeded(TRX_GL_FBO *fbo, int32_t width, int32_t height);

// Bind this FBO for rendering (GL_FRAMEBUFFER).
void TRX_GL_FBO_Bind(const TRX_GL_FBO *fbo);

// Bind the default framebuffer (0).
void TRX_GL_FBO_Unbind(void);
