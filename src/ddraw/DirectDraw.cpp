#include "ddraw/DirectDraw.hpp"

#include "ddraw/DirectDrawSurface.hpp"

namespace glrage {
namespace ddraw {

HRESULT DirectDraw::CreateSurface(
    LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface)
{
    *lplpDDSurface = new DirectDrawSurface(*this, m_renderer, lpDDSurfaceDesc);
    return DD_OK;
}

}
}
