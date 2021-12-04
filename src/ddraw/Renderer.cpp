#include "ddraw/Renderer.hpp"

#include "glrage_gl/Shader.hpp"
#include "glrage_gl/utils.h"
#include "glrage_util/StringUtils.hpp"

namespace glrage {
namespace ddraw {

Renderer::Renderer()
{
    GLRage_GLBuffer_Init(&m_surfaceBuffer, GL_ARRAY_BUFFER);
    GLRage_GLBuffer_Bind(&m_surfaceBuffer);
    GLfloat verts[] = { 0.0, 0.0, 1.0, 0.0, 0.0, 1.0,
                        0.0, 1.0, 1.0, 0.0, 1.0, 1.0 };
    GLRage_GLBuffer_Data(
        &m_surfaceBuffer, sizeof(verts), verts, GL_STATIC_DRAW);

    GLRage_GLVertexArray_Init(&m_surfaceFormat);
    GLRage_GLVertexArray_Bind(&m_surfaceFormat);
    GLRage_GLVertexArray_Attribute(
        &m_surfaceFormat, 0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLRage_GLTexture_Init(&m_surfaceTexture, GL_TEXTURE_2D);

    GLRage_GLSampler_Init(&m_sampler);
    GLRage_GLSampler_Bind(&m_sampler, 0);
    GLRage_GLSampler_Parameteri(&m_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRage_GLSampler_Parameteri(&m_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GLRage_GLSampler_Parameteri(
        &m_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    GLRage_GLSampler_Parameteri(
        &m_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    std::string basePath = m_context.getBasePath();
    m_program.attach(gl::Shader(GL_VERTEX_SHADER)
                         .fromFile(basePath + "\\shaders\\ddraw.vsh"));
    m_program.attach(gl::Shader(GL_FRAGMENT_SHADER)
                         .fromFile(basePath + "\\shaders\\ddraw.fsh"));
    m_program.link();
    m_program.fragmentData("fragColor");

    GLRage_GLCheckError();
}

Renderer::~Renderer()
{
    GLRage_GLVertexArray_Close(&m_surfaceFormat);
    GLRage_GLBuffer_Close(&m_surfaceBuffer);
    GLRage_GLTexture_Close(&m_surfaceTexture);
    GLRage_GLSampler_Close(&m_sampler);
}

void Renderer::upload(DDSURFACEDESC &desc, std::vector<uint8_t> &data)
{
    uint32_t width = desc.dwWidth;
    uint32_t height = desc.dwHeight;
    uint8_t *bits = &data[0];

    GLenum tex_format = GL_BGRA;
    GLenum tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    GLRage_GLTexture_Bind(&m_surfaceTexture);

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
    m_program.bind();
    GLRage_GLBuffer_Bind(&m_surfaceBuffer);
    GLRage_GLVertexArray_Bind(&m_surfaceFormat);
    GLRage_GLTexture_Bind(&m_surfaceTexture);
    GLRage_GLSampler_Bind(&m_sampler, 0);

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

    GLRage_GLCheckError();
}

}
}
