#include "ddraw/DirectDraw.hpp"

#include "ddraw/DirectDrawSurface.hpp"
#include "glrage_util/Logger.hpp"

namespace glrage {
namespace ddraw {

HRESULT DirectDraw::CreateSurface(
    LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface)
{
    *lplpDDSurface = new DirectDrawSurface(*this, m_renderer, lpDDSurfaceDesc);
    return DD_OK;
}

HRESULT DirectDraw::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc)
{
    LPDDSURFACEDESC desc = lpDDSurfaceDesc;
    desc->ddpfPixelFormat.dwRGBBitCount = 32;
    desc->dwWidth = m_context.getDisplayWidth();
    desc->dwHeight = m_context.getDisplayHeight();
    desc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    return DD_OK;
}

HRESULT DirectDraw::SetDisplayMode(DWORD dwWidth, DWORD dwHeight)
{
    m_context.setDisplaySize(dwWidth, dwHeight);
    return DD_OK;
}

}
}
