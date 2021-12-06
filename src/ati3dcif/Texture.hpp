#pragma once

#include "ati3dcif/ATI3DCIF.h"
#include "gfx/gl/texture.h"

#include <vector>

namespace glrage {
namespace cif {

class Texture {
public:
    Texture();
    ~Texture();
    void bind();
    GLenum target();
    GLuint id();

    bool load(C3D_PTMAP tmap);

private:
    GFX_GL_Texture m_GLTexture;
};

}
}
