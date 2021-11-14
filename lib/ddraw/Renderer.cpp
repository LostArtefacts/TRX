#include "Renderer.hpp"

#include <glrage_gl/Shader.hpp>
#include <glrage_gl/Utils.hpp>

// #include "openssl/md5.h"
#include <glrage_util/StringUtils.hpp>

namespace glrage {
namespace ddraw {

Renderer::Renderer()
    : m_surfaceBuffer(GL_ARRAY_BUFFER)
{
    // configure buffer
    m_surfaceBuffer.bind();
    GLfloat verts[] = {
        0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0};
    m_surfaceBuffer.data(sizeof(verts), verts, GL_STATIC_DRAW);

    m_surfaceFormat.bind();
    m_surfaceFormat.attribute(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // configure sampler
    std::string filterMethod =
        m_config.getString("directdraw.filter_method", "linear");
    GLint filterMethodEnum;

    if (filterMethod == "nearest") {
        filterMethodEnum = GL_NEAREST;
    } else {
        filterMethodEnum = GL_LINEAR;
    }

    m_sampler.bind(0);
    m_sampler.parameteri(GL_TEXTURE_MAG_FILTER, filterMethodEnum);
    m_sampler.parameteri(GL_TEXTURE_MIN_FILTER, filterMethodEnum);

    // configure shaders
    std::string basePath = m_context.getBasePath();
    m_program.attach(gl::Shader(GL_VERTEX_SHADER)
                         .fromFile(basePath + "\\shaders\\ddraw.vsh"));
    m_program.attach(gl::Shader(GL_FRAGMENT_SHADER)
                         .fromFile(basePath + "\\shaders\\ddraw.fsh"));
    m_program.link();
    m_program.fragmentData("fragColor");

    gl::Utils::checkError(__FUNCTION__);
}

void Renderer::upload(DDSURFACEDESC& desc, std::vector<uint8_t>& data)
{
    std::string texDir = m_config.getString("patch.texture_directory", "");
    uint32_t width = desc.dwWidth;
    uint32_t height = desc.dwHeight;
    uint8_t* bits = &data[0];
    GLenum tex_format = TEX_FORMAT;
    GLenum tex_type = TEX_TYPE;
    m_surfaceTexture.bind();

    // if (texDir.length() > 0 && desc.dwWidth == TITLE_WIDTH && desc.dwHeight
    // == TITLE_HEIGHT)
    // {
    //     uint8_t md5sum[16];
    //     MD5(&data[0], data.size(), md5sum);
    //     uint8_t *key = reinterpret_cast<uint8_t
    //     *>("\x4D\xD5\x68\xAD\xD2\x7E\x5B\xC2\x07\xF0\xD3\xC4\xB8\xEC\xCE\x67");
    //     if (memcmp(md5sum, key, 16) == 0)
    //     {
    //         if (m_overrideImage.IsNull())
    //         {
    //             std::string fileName = texDir + "\\TITLEH.PNG";
    //             std::string wFileName(fileName.begin(), fileName.end());
    //             CImage image;
    //             if (SUCCEEDED(image.Load(wFileName.c_str())))
    //             {
    //                 m_overrideImage.Create(image.GetWidth(),
    //                 -image.GetHeight(), 24, 0);
    //                 image.BitBlt(m_overrideImage.GetDC(), 0, 0);
    //             }
    //         }

    //         if (!m_overrideImage.IsNull())
    //         {
    //             bits = reinterpret_cast<uint8_t *>(m_overrideImage.GetBits());
    //             width = m_overrideImage.GetWidth();
    //             height = m_overrideImage.GetHeight();
    //             tex_format = GL_BGR;
    //             tex_type = GL_UNSIGNED_BYTE;
    //         }
    //     }
    // }

    // update buffer if the size is unchanged, otherwise create a new one
    if (width != m_width || height != m_height) {
        m_width = width;
        m_height = height;
        glTexImage2D(GL_TEXTURE_2D,
            0,
            TEX_INTERNAL_FORMAT,
            m_width,
            m_height,
            0,
            tex_format,
            tex_type,
            bits);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D,
            0,
            0,
            0,
            m_width,
            m_height,
            tex_format,
            tex_type,
            bits);
    }
}

void Renderer::render()
{
    m_program.bind();
    m_surfaceBuffer.bind();
    m_surfaceFormat.bind();
    m_surfaceTexture.bind();
    m_sampler.bind(0);

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

    gl::Utils::checkError(__FUNCTION__);
}

} // namespace ddraw
} // namespace glrage
