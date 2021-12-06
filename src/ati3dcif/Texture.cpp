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

bool Texture::load(const void *data, int width, int height)
{
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE,
        data);

    bind();
    glGenerateMipmap(GL_TEXTURE_2D);

    GFX_GL_CheckError();
    return true;
}

}
}
