#include "ati3dcif/TransDelay.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace glrage {
namespace cif {

void TransDelay::setTexture(std::shared_ptr<Texture> texture)
{
    auto group = std::make_shared<TexGroup>();
    group->texture = texture;
    m_store.push_back(group);
}

void TransDelay::setTexturingEnabled(bool enable)
{
    m_texturing_enabled = enable;
}

bool TransDelay::delayTriangle(C3D_VTCF *verts)
{
    if (m_texturing_enabled && m_store.size() > 0) {
        auto group = m_store.back();

        if (group->texture->isTranslucent()) {
            float xmin = FLT_MAX;
            float ymin = FLT_MAX;
            float xmax = FLT_MIN;
            float ymax = FLT_MIN;
            for (int i = 0; i < 3; i++) {
                float x = 32.0f * verts[i].s / verts[i].w;
                float y = 32.0f * verts[i].t / verts[i].w;
                if (x < xmin)
                    xmin = x;
                if (x > xmax)
                    xmax = x;
                if (y < ymin)
                    ymin = y;
                if (y > ymax)
                    ymax = y;
            }
            int32_t ixmin = std::max(static_cast<int32_t>(floorf(xmin)), 0);
            int32_t iymin = std::max(static_cast<int32_t>(floorf(ymin)), 0);
            int32_t ixmax = std::min(static_cast<int32_t>(ceilf(xmax)), 32);
            int32_t iymax = std::min(static_cast<int32_t>(ceilf(ymax)), 32);
            auto map = group->texture->translucencyMap();
            for (int32_t iy = iymin; iy < iymax; iy++) {
                for (int32_t ix = ixmin; ix < ixmax; ix++) {
                    if (map[iy * 32 + ix] != 0) {
                        for (int i = 0; i < 3; i++)
                            group->vtcBuffer.push_back(verts[i]);

                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void TransDelay::render(
    const std::function<void(std::vector<C3D_VTCF>)> &renderer)
{
    GLboolean texture2d = glIsEnabled(GL_TEXTURE_2D);
    if (!texture2d) {
        glEnable(GL_TEXTURE_2D);
    }

    GLboolean blend = glIsEnabled(GL_BLEND);
    if (!blend) {
        glEnable(GL_BLEND);
    }

    GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    if (!depthTest) {
        glEnable(GL_DEPTH_TEST);
    }

    GLint srcBlend, dstBlend;
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcBlend);
    glGetIntegerv(GL_BLEND_DST_RGB, &dstBlend);
    if (srcBlend != GL_SRC_ALPHA || dstBlend != GL_ONE_MINUS_SRC_ALPHA) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    for (auto it = m_store.begin(); it != m_store.end(); ++it) {
        auto group = *it;
        group->texture->bind();
        renderer(group->vtcBuffer);
    }

    m_store.clear();

    if (!texture2d) {
        glDisable(GL_TEXTURE_2D);
    }

    if (!blend) {
        glDisable(GL_BLEND);
    }

    if (!depthTest) {
        glDisable(GL_DEPTH_TEST);
    }

    if (srcBlend != GL_SRC_ALPHA || dstBlend != GL_ONE_MINUS_SRC_ALPHA) {
        glBlendFunc(srcBlend, dstBlend);
    }
}

}
}
