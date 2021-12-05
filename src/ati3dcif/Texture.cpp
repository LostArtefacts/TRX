#include "ati3dcif/Texture.hpp"

#include "ati3dcif/Error.hpp"
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

void Texture::load(C3D_PTMAP tmap, std::vector<C3D_PALETTENTRY> &palette)
{
    m_chromaKey = tmap->clrTexChromaKey;
    m_keyOnAlpha = false;
    m_is_translucent = false;

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

        case C3D_ETF_CI8: {
            uint8_t *src = static_cast<uint8_t *>(tmap->apvLevels[level]);

            std::vector<uint8_t> dst;
            // TODO: implement texture packs

            // Resolve indices to RGBA, which requires less code and is
            // faster than texture palettes in shaders.
            // Modern hardware really doesn't care about a few KB more
            // or less per texture anyway.
            dst.resize(size * 4);
            for (uint32_t i = 0; i < size; i++) {
                C3D_PALETTENTRY c = palette[src[i]];
                dst[i * 4 + 0] = c.r;
                dst[i * 4 + 1] = c.g;
                dst[i * 4 + 2] = c.b;
                dst[i * 4 + 3] = 0xff;
            }

            // upload texture data
            glTexImage2D(
                GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, &dst[0]);

            break;
        }

        default:
            throw Error(
                "Unsupported texture format: "
                    + std::to_string(tmap->eTexFormat),
                C3D_EC_NOTIMPYET);
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
}

C3D_COLOR &Texture::chromaKey()
{
    return m_chromaKey;
}

bool Texture::keyOnAlpha()
{
    return m_keyOnAlpha;
}

bool Texture::isTranslucent()
{
    return m_is_translucent;
}

std::vector<uint8_t> &Texture::translucencyMap()
{
    return m_translucency_map;
}

}
}
