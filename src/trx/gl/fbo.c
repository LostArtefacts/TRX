#include <trx/gl/fbo.h>

#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/viewport.h>
#include <trx/gl/buffer.h>
#include <trx/gl/context.h>
#include <trx/gl/program.h>
#include <trx/gl/texture.h>
#include <trx/gl/utils.h>
#include <trx/gl/vertex_array.h>

#include <GL/glew.h>

static int32_t M_ClampSamples(const int32_t samples)
{
    GLint max_samples = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
    return MAX(1, MIN(samples, (int32_t)max_samples));
}

void TRX_GL_FBO_Init(
    TRX_GL_FBO *const fbo, const int32_t width, const int32_t height,
    const int32_t samples, const GLint internal_format, const GLenum format,
    const bool with_depth_stencil)
{
    fbo->width = width;
    fbo->height = height;
    fbo->samples = M_ClampSamples(samples);
    fbo->internal_format = internal_format;
    fbo->format = format;
    fbo->with_depth_stencil = with_depth_stencil;

    ASSERT(width > 0);
    ASSERT(height > 0);

    const bool is_multisample = fbo->samples > 1;
    const GLenum target =
        is_multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

    // Allocate color texture (no mipmaps for FBO attachments).
    TRX_GL_Texture_Init(&fbo->texture, target);
    glBindTexture(target, fbo->texture.id);
    TRX_GL_CheckError();
    if (is_multisample) {
        // Filtering and wrapping mean nothing to a multisample texture; it is
        // resolved rather than sampled.
        glTexImage2DMultisample(
            target, fbo->samples, internal_format, width, height, GL_TRUE);
    } else {
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(
            target, 0, internal_format, width, height, 0, format,
            GL_UNSIGNED_BYTE, nullptr);
    }
    glClearColor(0.0, 0.0, 0.0, 1.0);
    TRX_GL_CheckError();

    glGenFramebuffers(1, &fbo->fbo);
    TRX_GL_CheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
    TRX_GL_CheckError();

    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, fbo->texture.id, 0);
    TRX_GL_CheckError();

    // direct draw to color attachment 0.
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    TRX_GL_CheckError();

    if (with_depth_stencil) {
        glGenRenderbuffers(1, &fbo->rbo);
        TRX_GL_CheckError();
        glBindRenderbuffer(GL_RENDERBUFFER, fbo->rbo);
        TRX_GL_CheckError();
        if (is_multisample) {
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER, fbo->samples, GL_DEPTH24_STENCIL8, width,
                height);
        } else {
            glRenderbufferStorage(
                GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        }
        TRX_GL_CheckError();
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        TRX_GL_CheckError();
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
            fbo->rbo);
        TRX_GL_CheckError();
    } else {
        fbo->rbo = 0;
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("framebuffer is not complete!");
    }

    // A framebuffer can be read from before anything has drawn into it: the
    // frame a resize lands on is composited from the one before it.
    const GLfloat black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 0, black);
    TRX_GL_CheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TRX_GL_FBO_Close(TRX_GL_FBO *fbo)
{
    if (fbo->rbo) {
        glDeleteRenderbuffers(1, &fbo->rbo);
        fbo->rbo = 0;
    }
    if (fbo->fbo) {
        glDeleteFramebuffers(1, &fbo->fbo);
        fbo->fbo = 0;
    }
    TRX_GL_Texture_Close(&fbo->texture);
}

void TRX_GL_FBO_ResizeIfNeeded(
    TRX_GL_FBO *const fbo, const int32_t width, const int32_t height,
    const int32_t samples)
{
    if (width == fbo->width && height == fbo->height
        && M_ClampSamples(samples) == fbo->samples) {
        return;
    }

    const GLint internal_format = fbo->internal_format;
    const GLenum format = fbo->format;
    const bool with_depth_stencil = fbo->with_depth_stencil;

    TRX_GL_FBO_Close(fbo);
    TRX_GL_FBO_Init(
        fbo, width, height, samples, internal_format, format,
        with_depth_stencil);
}

void TRX_GL_FBO_Bind(const TRX_GL_FBO *const fbo)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
    TRX_GL_CheckError();
}

void TRX_GL_FBO_Unbind(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    TRX_GL_CheckError();
}
