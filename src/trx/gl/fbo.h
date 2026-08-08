// Framebuffer object abstraction for off-screen rendering.

#pragma once

#include <trx/game/viewport.h>
#include <trx/gl/context.h>
#include <trx/gl/texture.h>

#include <GL/glew.h>

// Off-screen framebuffer with a single color attachment and optional
// depth+stencil.
typedef struct {
    int32_t width;
    int32_t height;
    // Samples per pixel. Above one the color attachment is a multisample
    // texture, which no shader here samples directly: it has to be resolved
    // into a single-sample framebuffer first.
    int32_t samples;
    GLuint fbo;
    GLuint rbo;
    GLint internal_format;
    GLenum format;
    bool with_depth_stencil;
    TRX_GL_TEXTURE texture;
} TRX_GL_FBO;

// Initialize an off-screen FBO. A sample count above one is clamped to what
// the driver reports it supports.
void TRX_GL_FBO_Init(
    TRX_GL_FBO *fbo, int32_t width, int32_t height, int32_t samples,
    GLint internal_format, GLenum format, bool with_depth_stencil);

// Close and free the GL resources.
void TRX_GL_FBO_Close(TRX_GL_FBO *fbo);

// Resize the FBO attachments, or rebuild them for a new sample count.
void TRX_GL_FBO_ResizeIfNeeded(
    TRX_GL_FBO *fbo, int32_t width, int32_t height, int32_t samples);

// Bind this FBO for rendering (GL_FRAMEBUFFER).
void TRX_GL_FBO_Bind(const TRX_GL_FBO *fbo);

// Bind the default framebuffer (0).
void TRX_GL_FBO_Unbind(void);
