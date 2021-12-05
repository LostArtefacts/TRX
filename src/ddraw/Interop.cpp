#include "ddraw/Interop.hpp"

#include "ddraw/2d_surface.h"
#include "gfx/2d/2d_renderer.h"
#include "gfx/context.h"
#include "log.h"

#include <cassert>
#include <string>

namespace glrage {
namespace ddraw {

extern "C" {

GFX_2D_Surface *MyIDirectDraw2_CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc)
{
    GFX_2D_Renderer *renderer = GFX_Context_GetRenderer2D();
    GFX_2D_Surface *surface = new GFX_2D_Surface();
    GFX_2D_Surface_Init(surface, renderer, lpDDSurfaceDesc);
    return surface;
}

GFX_2D_Surface *MyIDirectDrawSurface_GetAttachedSurface(GFX_2D_Surface *p)
{
    assert(p);
    return GFX_2D_Surface_GetAttachedSurface(p);
}

HRESULT MyIDirectDrawSurface2_Lock(
    GFX_2D_Surface *p, LPDDSURFACEDESC lpDDSurfaceDesc)
{
    assert(p);
    return GFX_2D_Surface_Lock(p, lpDDSurfaceDesc);
}

HRESULT MyIDirectDrawSurface2_Unlock(GFX_2D_Surface *p, LPVOID lp)
{
    assert(p);
    return GFX_2D_Surface_Unlock(p, lp);
}

HRESULT MyIDirectDrawSurface_Blt(
    GFX_2D_Surface *p, LPRECT lpDestRect, GFX_2D_Surface *lpDDSrcSurface,
    LPRECT lpSrcRect, DWORD dwFlags)
{
    assert(p);
    return GFX_2D_Surface_Blt(
        p, lpDestRect, lpDDSrcSurface, lpSrcRect, dwFlags);
}

void MyIDirectDrawSurface_Release(GFX_2D_Surface *p)
{
    assert(p);
    GFX_2D_Surface_Close(p);
    delete p;
}

HRESULT MyIDirectDrawSurface_Flip(GFX_2D_Surface *p)
{
    assert(p);
    return GFX_2D_Surface_Flip(p);
}
}

}
}
