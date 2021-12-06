#include "ati3dcif/Texture.hpp"

#include "gfx/gl/utils.h"
#include "log.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>

#undef max

namespace glrage {
namespace cif {

Texture::Texture()
{
    GFX_GL_Texture_Init(&m_GLTexture, GL_TEXTURE_2D);
}

Texture::~Texture()
{
    GFX_GL_Texture_Close(&m_GLTexture);
}

void Texture::bind()
{
    GFX_GL_Texture_Bind(&m_GLTexture);
}

GLenum Texture::target()
{
    return GFX_GL_Texture_Target(&m_GLTexture);
}

GLuint Texture::id()
{
    return m_GLTexture.id;
}

bool Texture::load(C3D_PTMAP tmap)
{
    m_chromaKey = tmap->clrTexChromaKey;

    // convert and generate texture for each level
    uint32_t width = 1 << tmap->u32MaxMapXSizeLg2;
    uint32_t height = 1 << tmap->u32MaxMapYSizeLg2;
    uint32_t size = width * height;

    uint32_t levels = 1;
    if (tmap->bMipMap) {
        levels = std::max(tmap->u32MaxMapXSizeLg2, tmap->u32MaxMapYSizeLg2) + 1;
    }

    for (uint32_t level = 0; level < levels; level++) {
        LOG_INFO("level %d (%dx%d)", level, width, height);

        // convert texture data
        switch (tmap->eTexFormat) {
        case C3D_ETF_RGB1555: {
            uint16_t *src = static_cast<uint16_t *>(tmap->apvLevels[level]);

            // toggle alpha bit, which has the opposite meaning in OpenGL
            for (uint32_t i = 0; i < size; i++) {
                src[i] ^= 1 << 15;
            }

            // upload texture data
            glTexImage2D(
                GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_BGRA,
                GL_UNSIGNED_SHORT_1_5_5_5_REV, tmap->apvLevels[level]);

            break;
        }

        case C3D_ETF_RGB332: {
            glTexImage2D(
                GL_TEXTURE_2D, level, GL_RGB, width, height, 0, GL_RGB,
                GL_UNSIGNED_BYTE_3_3_2, tmap->apvLevels[level]);
            break;
        }

        case C3D_ETF_RGB565: {
            glTexImage2D(
                GL_TEXTURE_2D, level, GL_RGB, width, height, 0, GL_RGB,
                GL_UNSIGNED_SHORT_5_6_5_REV, tmap->apvLevels[level]);
            break;
        }

        case C3D_ETF_RGB4444: {
            glTexImage2D(
                GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_BGRA,
                GL_UNSIGNED_SHORT_4_4_4_4_REV, tmap->apvLevels[level]);
            break;
        }

        case C3D_ETF_RGB8888: {
            glTexImage2D(
                GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_BGRA,
                GL_UNSIGNED_BYTE, tmap->apvLevels[level]);

            break;
        }

        default:
            LOG_ERROR("Unsupported texture format: %d", tmap->eTexFormat);
            return false;
        }

        // set dimensions for next level
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
        size = width * height;
    }

    // generate mipmaps automatically if the application doesn't provide any
    if (levels == 1) {
        bind();
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    // FIXME: sampler object overrides these parameters
    // if (tmap->u32Size > 68) {
    //    bind();

    //    // set static texture parameters
    //    if (tmap->bClampS) {
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
    //        GL_CLAMP_TO_EDGE);
    //    }

    //    if (tmap->bClampT) {
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
    //        GL_CLAMP_TO_EDGE);
    //    }
    //}

    GFX_GL_CheckError();
    return true;
}

C3D_COLOR &Texture::chromaKey()
{
    return m_chromaKey;
}

}
}
