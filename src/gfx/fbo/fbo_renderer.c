#include "gfx/fbo/fbo_renderer.h"

#include "gfx/context.h"
#include "gfx/gl/utils.h"
#include "log.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

void GFX_FBO_Renderer_Init(GFX_FBO_Renderer *renderer)
{
    LOG_INFO("");
    assert(renderer);

    int32_t fbo_width = GFX_Context_GetDisplayWidth();
    int32_t fbo_height = GFX_Context_GetDisplayHeight();

    GFX_GL_Buffer_Init(&renderer->buffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&renderer->buffer);
    GLfloat verts[] = { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0,
                        0.0, 1.0, 1.0, 0.0, 1.0, 1.0 };
    GFX_GL_Buffer_Data(&renderer->buffer, sizeof(verts), verts, GL_STATIC_DRAW);

    GFX_GL_VertexArray_Init(&renderer->vertex_array);
    GFX_GL_VertexArray_Bind(&renderer->vertex_array);
    GFX_GL_VertexArray_Attribute(
        &renderer->vertex_array, 0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GFX_GL_Texture_Init(&renderer->texture, GL_TEXTURE_2D);

    GFX_GL_Sampler_Init(&renderer->sampler);
    GFX_GL_Sampler_Bind(&renderer->sampler, 0);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    GFX_GL_Program_Init(&renderer->program);
    GFX_GL_Program_AttachShader(
        &renderer->program, GL_VERTEX_SHADER, "shaders/fbo.vsh");
    GFX_GL_Program_AttachShader(
        &renderer->program, GL_FRAGMENT_SHADER, "shaders/fbo.fsh");
    GFX_GL_Program_Link(&renderer->program);
    GFX_GL_Program_FragmentData(&renderer->program, "fragColor");

    glGenFramebuffers(1, &renderer->fbo);
    GFX_GL_CheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);
    GFX_GL_CheckError();

    GFX_GL_Texture_Load(
        &renderer->texture, NULL, fbo_width, fbo_height, GL_RGB, GL_RGB);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        renderer->texture.id, 0);
    GFX_GL_CheckError();

    glGenRenderbuffers(1, &renderer->rbo);
    GFX_GL_CheckError();

    glBindRenderbuffer(GL_RENDERBUFFER, renderer->rbo);
    GFX_GL_CheckError();

    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbo_width, fbo_height);
    GFX_GL_CheckError();

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GFX_GL_CheckError();

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
        renderer->rbo);
    GFX_GL_CheckError();

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("framebuffer is not complete!");
    }
}

void GFX_FBO_Renderer_Close(GFX_FBO_Renderer *renderer)
{
    LOG_INFO("");
    assert(renderer);
    if (!renderer->fbo) {
        return;
    }
    glDeleteFramebuffers(1, &renderer->fbo);
    renderer->fbo = 0;
    GFX_GL_VertexArray_Close(&renderer->vertex_array);
    GFX_GL_Buffer_Close(&renderer->buffer);
    GFX_GL_Texture_Close(&renderer->texture);
    GFX_GL_Sampler_Close(&renderer->sampler);
    GFX_GL_Program_Close(&renderer->program);
}

void GFX_FBO_Renderer_Render(GFX_FBO_Renderer *renderer)
{
    assert(renderer);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GFX_GL_CheckError();

    GFX_GL_Program_Bind(&renderer->program);
    GFX_GL_Buffer_Bind(&renderer->buffer);
    GFX_GL_VertexArray_Bind(&renderer->vertex_array);
    GFX_GL_Texture_Bind(&renderer->texture);
    GFX_GL_Sampler_Bind(&renderer->sampler, 0);

    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MAG_FILTER,
        renderer->is_smoothing_enabled ? GL_LINEAR : GL_NEAREST);
    GFX_GL_Sampler_Parameteri(
        &renderer->sampler, GL_TEXTURE_MIN_FILTER,
        renderer->is_smoothing_enabled ? GL_LINEAR : GL_NEAREST);

    GLboolean blend = glIsEnabled(GL_BLEND);
    if (blend) {
        glDisable(GL_BLEND);
    }

    GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
    if (depth_test) {
        glDisable(GL_DEPTH_TEST);
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);
    GFX_GL_CheckError();

    if (blend) {
        glEnable(GL_BLEND);
    }

    if (depth_test) {
        glEnable(GL_DEPTH_TEST);
    }
    GFX_GL_CheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);
    GFX_GL_CheckError();
}

void GFX_FBO_Renderer_SetSmoothingEnabled(
    GFX_FBO_Renderer *renderer, bool is_enabled)
{
    assert(renderer);
    renderer->is_smoothing_enabled = is_enabled;
}

void GFX_FBO_Renderer_Bind(const GFX_FBO_Renderer *renderer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->fbo);
}

void GFX_FBO_Renderer_Unbind(const GFX_FBO_Renderer *renderer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
