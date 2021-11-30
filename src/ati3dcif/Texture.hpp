#pragma once

#include "ati3dcif/ATI3DCIF.h"
#include "glrage/GLRage.hpp"
#include "glrage_gl/Texture.hpp"

#include <vector>

namespace glrage {
namespace cif {

class Texture : public gl::Texture {
public:
    static const uint32_t TRANS_MAP_DIM = 32;
    static const uint32_t TRANS_TEX_DIM = 1024;
    static const uint32_t TRANS_MAP_FACTOR = 32;
    Texture();
    ~Texture();
    void load(C3D_PTMAP tmap, std::vector<C3D_PALETTENTRY> &palette);
    C3D_COLOR &chromaKey();
    bool keyOnAlpha();
    bool isTranslucent();
    std::vector<uint8_t> &translucencyMap();

private:
    C3D_COLOR m_chromaKey = { 0, 0, 0, 0 };
    bool m_keyOnAlpha = false;
    std::vector<uint8_t> m_translucency_map;
    bool m_is_translucent = false;
};

}
}
