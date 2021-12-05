#include "gfx/2d/2d_renderer.h"

#include "gfx/gl/utils.h"

void GFX_2D_Renderer_Init(GFX_2D_Renderer *renderer)
{
    GFX_GL_Buffer_Init(&renderer->surface_buffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&renderer->surface_buffer);
    GLfloat verts[] = { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0,
                        0.0, 1.0, 1.0, 0.0, 1.0, 1.0 };
    GFX_GL_Buffer_Data(
        &renderer->surface_buffer, sizeof(verts), verts, GL_STATIC_DRAW);

    GFX_GL_VertexArray_Init(&renderer->surface_format);
    GFX_GL_VertexArray_Bind(&renderer->surface_format);
    GFX_GL_VertexArray_Attribute(
        &renderer->surface_format, 0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GFX_GL_Texture_Init(&renderer->surface_texture, GL_TEXTURE_2D);

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
        &renderer->program, GL_VERTEX_SHADER, "shaders\\ddraw.vsh");
    GFX_GL_Program_AttachShader(
        &renderer->program, GL_FRAGMENT_SHADER, "shaders\\ddraw.fsh");
    GFX_GL_Program_Link(&renderer->program);
    GFX_GL_Program_FragmentData(&renderer->program, "fragColor");

    GFX_GL_CheckError();
}

void GFX_2D_Renderer_Close(GFX_2D_Renderer *renderer)
{
    GFX_GL_VertexArray_Close(&renderer->surface_format);
    GFX_GL_Buffer_Close(&renderer->surface_buffer);
    GFX_GL_Texture_Close(&renderer->surface_texture);
    GFX_GL_Sampler_Close(&renderer->sampler);
    GFX_GL_Program_Close(&renderer->program);
}

void GFX_2D_Renderer_Upload(
    GFX_2D_Renderer *renderer, GFX_2D_SurfaceDesc *desc, const uint8_t *data)
{
    uint32_t width = desc->width;
    uint32_t height = desc->height;

    GLenum tex_format = GL_BGRA;
    GLenum tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    GFX_GL_Texture_Bind(&renderer->surface_texture);

    // TODO: implement texture packs

    // update buffer if the size is unchanged, otherwise create a new one
    if (width != renderer->width || height != renderer->height) {
        renderer->width = width;
        renderer->height = height;
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, renderer->width, renderer->height, 0,
            tex_format, tex_type, data);
    } else {
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, renderer->width, renderer->height,
            tex_format, tex_type, data);
    }
}

void GFX_2D_Renderer_Render(GFX_2D_Renderer *renderer)
{
    GFX_GL_Program_Bind(&renderer->program);
    GFX_GL_Buffer_Bind(&renderer->surface_buffer);
    GFX_GL_VertexArray_Bind(&renderer->surface_format);
    GFX_GL_Texture_Bind(&renderer->surface_texture);
    GFX_GL_Sampler_Bind(&renderer->sampler, 0);

    GLboolean texture2d = glIsEnabled(GL_TEXTURE_2D);
    if (!texture2d) {
        glEnable(GL_TEXTURE_2D);
    }

    GLboolean blend = glIsEnabled(GL_BLEND);
    if (blend) {
        glDisable(GL_BLEND);
    }

    GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    if (depthTest) {
        glDisable(GL_DEPTH_TEST);
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);

    if (!texture2d) {
        glDisable(GL_TEXTURE_2D);
    }

    if (blend) {
        glEnable(GL_BLEND);
    }

    if (depthTest) {
        glEnable(GL_DEPTH_TEST);
    }

    GFX_GL_CheckError();
}
