#include "ddraw/Interop.hpp"

#include "ddraw/DirectDrawSurface.hpp"
#include "ddraw/2d_renderer.h"
#include "log.h"

#include <cassert>
#include <string>

namespace glrage {
namespace ddraw {

extern "C" {

static GFX_2D_Renderer *m_Renderer2D;

HRESULT MyDirectDrawCreate()
{
    if (!m_Renderer2D) {
        m_Renderer2D = new GFX_2D_Renderer();
        GFX_2D_Renderer_Init(m_Renderer2D);
    }
    return DD_OK;
}

void MyIDirectDraw_Release()
{
    if (m_Renderer2D) {
        GFX_2D_Renderer_Close(m_Renderer2D);
        delete m_Renderer2D;
    }
}

void MyIDirectDraw2_CreateSurface(
    LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface)
{
    *lplpDDSurface = new DirectDrawSurface(m_Renderer2D, lpDDSurfaceDesc);
}

void MyIDirectDrawSurface_GetAttachedSurface(
    LPDIRECTDRAWSURFACE p, LPDIRECTDRAWSURFACE *lplpDDAttachedSurface)
{
    assert(p);
    reinterpret_cast<DirectDrawSurface *>(p)->GetAttachedSurface(
        lplpDDAttachedSurface);
}

HRESULT MyIDirectDrawSurface2_Lock(
    LPDIRECTDRAWSURFACE p, LPDDSURFACEDESC lpDDSurfaceDesc)
{
    assert(p);
    return reinterpret_cast<DirectDrawSurface *>(p)->Lock(lpDDSurfaceDesc);
}

HRESULT MyIDirectDrawSurface2_Unlock(LPDIRECTDRAWSURFACE p, LPVOID lp)
{
    assert(p);
    return reinterpret_cast<DirectDrawSurface *>(p)->Unlock(lp);
}

HRESULT MyIDirectDrawSurface_Blt(
    LPDIRECTDRAWSURFACE p, LPRECT lpDestRect,
    LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags)
{
    assert(p);
    return reinterpret_cast<DirectDrawSurface *>(p)->Blt(
        lpDestRect, lpDDSrcSurface, lpSrcRect, dwFlags);
}

void MyIDirectDrawSurface_Release(LPDIRECTDRAWSURFACE p)
{
    assert(p);
    delete reinterpret_cast<DirectDrawSurface *>(p);
}

HRESULT MyIDirectDrawSurface_Flip(LPDIRECTDRAWSURFACE p)
{
    assert(p);
    return reinterpret_cast<DirectDrawSurface *>(p)->Flip();
}
}

}
}
