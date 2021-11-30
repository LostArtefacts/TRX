#pragma once

#include "ddraw/DirectDraw.hpp"
#include "ddraw/Renderer.hpp"
#include "ddraw/ddraw.h"
#include "glrage/GLRage.hpp"

#include <cstdint>
#include <vector>

namespace glrage {
namespace ddraw {

class DirectDrawSurface {
public:
    DirectDrawSurface(
        DirectDraw &lpDD, Renderer &renderer, LPDDSURFACEDESC lpDDSurfaceDesc);
    virtual ~DirectDrawSurface() = default;

    HRESULT Blt(
        LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect,
        DWORD dwFlags);
    HRESULT Flip();
    HRESULT GetAttachedSurface(
        LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE *lplpDDAttachedSurface);
    HRESULT GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
    HRESULT GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc);
    HRESULT Lock(LPDDSURFACEDESC lpDDSurfaceDesc);
    HRESULT Unlock(LPVOID lp);

private:
    Context &m_context = GLRage::getContext();
    Renderer &m_renderer;
    std::vector<uint8_t> m_buffer;
    DDSURFACEDESC m_desc;
    std::unique_ptr<DirectDrawSurface> m_backBuffer = nullptr;
    bool m_locked = false;
    bool m_dirty = false;
};

}
}
