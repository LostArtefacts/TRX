#include "ddraw/Renderer.hpp"

#include "gfx/gl/utils.h"
#include "glrage_util/StringUtils.hpp"

namespace glrage {
namespace ddraw {

Renderer::Renderer()
{
    GFX_GL_Buffer_Init(&m_surfaceBuffer, GL_ARRAY_BUFFER);
    GFX_GL_Buffer_Bind(&m_surfaceBuffer);
    GLfloat verts[] = { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0,
                        0.0, 1.0, 1.0, 0.0, 1.0, 1.0 };
    GFX_GL_Buffer_Data(&m_surfaceBuffer, sizeof(verts), verts, GL_STATIC_DRAW);

    GFX_GL_VertexArray_Init(&m_surfaceFormat);
    GFX_GL_VertexArray_Bind(&m_surfaceFormat);
    GFX_GL_VertexArray_Attribute(
        &m_surfaceFormat, 0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GFX_GL_Texture_Init(&m_surfaceTexture, GL_TEXTURE_2D);

    GFX_GL_Sampler_Init(&m_sampler);
    GFX_GL_Sampler_Bind(&m_sampler, 0);
    GFX_GL_Sampler_Parameteri(&m_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GFX_GL_Sampler_Parameteri(&m_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GFX_GL_Sampler_Parameteri(
        &m_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    GFX_GL_Sampler_Parameteri(
        &m_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    GFX_GL_Program_Init(&m_program);
    GFX_GL_Program_AttachShader(
        &m_program, GL_VERTEX_SHADER, "shaders\\ddraw.vsh");
    GFX_GL_Program_AttachShader(
        &m_program, GL_FRAGMENT_SHADER, "shaders\\ddraw.fsh");
    GFX_GL_Program_Link(&m_program);
    GFX_GL_Program_FragmentData(&m_program, "fragColor");

    GFX_GL_CheckError();
}

Renderer::~Renderer()
{
    GFX_GL_VertexArray_Close(&m_surfaceFormat);
    GFX_GL_Buffer_Close(&m_surfaceBuffer);
    GFX_GL_Texture_Close(&m_surfaceTexture);
    GFX_GL_Sampler_Close(&m_sampler);
    GFX_GL_Program_Close(&m_program);
}

void Renderer::upload(DDSURFACEDESC &desc, std::vector<uint8_t> &data)
{
    uint32_t width = desc.dwWidth;
    uint32_t height = desc.dwHeight;
    uint8_t *bits = &data[0];

    GLenum tex_format = GL_BGRA;
    GLenum tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    GFX_GL_Texture_Bind(&m_surfaceTexture);

    // TODO: implement texture packs

    // update buffer if the size is unchanged, otherwise create a new one
    if (width != m_width || height != m_height) {
        m_width = width;
        m_height = height;
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, tex_format,
            tex_type, bits);
    } else {
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, tex_format, tex_type,
            bits);
    }
}

void Renderer::render()
{
    GFX_GL_Program_Bind(&m_program);
    GFX_GL_Buffer_Bind(&m_surfaceBuffer);
    GFX_GL_VertexArray_Bind(&m_surfaceFormat);
    GFX_GL_Texture_Bind(&m_surfaceTexture);
    GFX_GL_Sampler_Bind(&m_sampler, 0);

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

}
}
