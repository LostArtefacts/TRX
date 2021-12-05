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

void MyIDirectDraw2_CreateSurface(
    LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface)
{
    GFX_2D_Renderer *renderer = GFX_Context_GetRenderer2D();
    GFX_2D_Surface *surface = new GFX_2D_Surface();
    *lplpDDSurface = surface;
    GFX_2D_Surface_Init(surface, renderer, lpDDSurfaceDesc);
}

void MyIDirectDrawSurface_GetAttachedSurface(
    LPDIRECTDRAWSURFACE p, LPDIRECTDRAWSURFACE *lplpDDAttachedSurface)
{
    assert(p);
    GFX_2D_Surface_GetAttachedSurface(
        reinterpret_cast<GFX_2D_Surface *>(p), lplpDDAttachedSurface);
}

HRESULT MyIDirectDrawSurface2_Lock(
    LPDIRECTDRAWSURFACE p, LPDDSURFACEDESC lpDDSurfaceDesc)
{
    assert(p);
    return GFX_2D_Surface_Lock(
        reinterpret_cast<GFX_2D_Surface *>(p), lpDDSurfaceDesc);
}

HRESULT MyIDirectDrawSurface2_Unlock(LPDIRECTDRAWSURFACE p, LPVOID lp)
{
    assert(p);
    return GFX_2D_Surface_Unlock(reinterpret_cast<GFX_2D_Surface *>(p), lp);
}

HRESULT MyIDirectDrawSurface_Blt(
    LPDIRECTDRAWSURFACE p, LPRECT lpDestRect,
    LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags)
{
    assert(p);
    return GFX_2D_Surface_Blt(
        reinterpret_cast<GFX_2D_Surface *>(p), lpDestRect, lpDDSrcSurface,
        lpSrcRect, dwFlags);
}

void MyIDirectDrawSurface_Release(LPDIRECTDRAWSURFACE p)
{
    assert(p);
    GFX_2D_Surface_Close(reinterpret_cast<GFX_2D_Surface *>(p));
    delete reinterpret_cast<GFX_2D_Surface *>(p);
}

HRESULT MyIDirectDrawSurface_Flip(LPDIRECTDRAWSURFACE p)
{
    assert(p);
    return GFX_2D_Surface_Flip(reinterpret_cast<GFX_2D_Surface *>(p));
}
}

}
}
