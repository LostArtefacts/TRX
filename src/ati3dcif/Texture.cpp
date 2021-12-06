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
        glTexImage2D(
            GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_BGRA,
            GL_UNSIGNED_BYTE, tmap->apvLevels[level]);

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

    GFX_GL_CheckError();
    return true;
}

C3D_COLOR &Texture::chromaKey()
{
    return m_chromaKey;
}

}
}
