#pragma once

#include "ddraw/Renderer.hpp"
#include "ddraw/ddraw.h"
#include "glrage/Context.hpp"

#include <cstdint>
#include <memory>

namespace glrage {
namespace ddraw {

class DirectDraw {
public:
    virtual ~DirectDraw() = default;

    HRESULT CreateSurface(
        LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface);
    HRESULT GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc);
    HRESULT SetDisplayMode(DWORD dwWidth, DWORD dwHeight);

private:
    Context &m_context = Context::instance();
    Renderer m_renderer;
};

}
}
