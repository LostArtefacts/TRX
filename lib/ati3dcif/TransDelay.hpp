#pragma once

#include <ati3dcif/ATI3DCIF.h>

#include "Texture.hpp"

#include <functional>
#include <memory>

namespace glrage {
namespace cif {

// Delays the drawing of primitives that exhibit translucency,
// so as to render them over the top of non-translucent primitives.
class TransDelay
{
public:
    void setTexture(std::shared_ptr<Texture> texture);
    void setTexturingEnabled(bool enabled);
    bool delayTriangle(C3D_VTCF* verts);
    void render(const std::function<void(std::vector<C3D_VTCF>)>& renderer);

private:
    struct TexGroup
    {
        std::shared_ptr<Texture> texture;
        std::vector<C3D_VTCF> vtcBuffer;
    };
    std::vector<std::shared_ptr<TexGroup>> m_store;
    bool m_texturing_enabled = false;
};

} // namespace cif
} // namespace glrage
