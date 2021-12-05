#pragma once

#include "ddraw/2d_renderer.h"
#include "ddraw/ddraw.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace glrage {
namespace ddraw {

class DirectDrawSurface {
public:
    DirectDrawSurface(
        GFX_2D_Renderer *renderer, LPDDSURFACEDESC lpDDSurfaceDesc);
    virtual ~DirectDrawSurface() = default;

    HRESULT Blt(
        LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect,
        DWORD dwFlags);
    HRESULT Flip();
    void GetAttachedSurface(LPDIRECTDRAWSURFACE *lplpDDAttachedSurface);
    HRESULT GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
    HRESULT GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc);
    HRESULT Lock(LPDDSURFACEDESC lpDDSurfaceDesc);
    HRESULT Unlock(LPVOID lp);

private:
    GFX_2D_Renderer *m_renderer;
    std::vector<uint8_t> m_buffer;
    DDSURFACEDESC m_desc;
    std::unique_ptr<DirectDrawSurface> m_backBuffer = nullptr;
    bool m_locked = false;
    bool m_dirty = false;
};

}
}
