#pragma once
#include "ati3dcif.hpp"
#include "Texture.hpp"

#include <memory>
#include <functional>

namespace glrage {
namespace cif {

// Delays the drawing of primitives that exhibit translucency,
// so as to render them over the top of non-translucent primitives.
class TransDelay
{
public:
    void setTexture(std::shared_ptr<Texture> texture);
    void setTexturingEnabled(bool enabled);
    bool delayTriangle(C3D_VTCF *verts);
    void render(const std::function<void(std::vector<C3D_VTCF>)>& renderer);

private:
    struct TexGroup
    {
        std::shared_ptr<Texture> texture;
        std::vector<C3D_VTCF> vtcBuffer;
    };
    std::vector<std::shared_ptr<TexGroup>> m_store;
    bool m_texturing_enabled;
};

} // namespace cif
} // namespace glrage
