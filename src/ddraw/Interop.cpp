#include "ddraw/Interop.hpp"

#include "ddraw/DirectDraw.hpp"
#include "ddraw/DirectDrawSurface.hpp"
#include "glrage/GLRage.hpp"
#include "glrage_util/ErrorUtils.hpp"

#include <cassert>
#include <string>

namespace glrage {
namespace ddraw {

extern "C" {

HRESULT MyDirectDrawCreate(LPDIRECTDRAW *lplpDD)
{
    Context &context = GLRage::getContext();

    try {
        *lplpDD = new DirectDraw();
    } catch (const std::exception &ex) {
        ErrorUtils::warning(ex);
        return DDERR_GENERIC;
    }

    return DD_OK;
}

HRESULT MyIDirectDraw_Release(LPDIRECTDRAW p)
{
    assert(p);
    delete reinterpret_cast<DirectDraw *>(p);
    return DD_OK;
}

HRESULT MyIDirectDraw_SetDisplayMode(
    LPDIRECTDRAW p, DWORD dwWidth, DWORD dwHeight)
{
    assert(p);
    return reinterpret_cast<DirectDraw *>(p)->SetDisplayMode(dwWidth, dwHeight);
}

HRESULT MyIDirectDraw2_CreateSurface(
    LPDIRECTDRAW p, LPDDSURFACEDESC lpDDSurfaceDesc,
    LPDIRECTDRAWSURFACE *lplpDDSurface)
{
    assert(p);
    return reinterpret_cast<DirectDraw *>(p)->CreateSurface(
        lpDDSurfaceDesc, lplpDDSurface);
}

HRESULT MyIDirectDrawSurface_GetAttachedSurface(
    LPDIRECTDRAWSURFACE p, LPDDSCAPS lpDDSCaps,
    LPDIRECTDRAWSURFACE *lplpDDAttachedSurface)
{
    assert(p);
    return reinterpret_cast<DirectDrawSurface *>(p)->GetAttachedSurface(
        lpDDSCaps, lplpDDAttachedSurface);
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

HRESULT MyIDirectDrawSurface_Release(LPDIRECTDRAWSURFACE p)
{
    assert(p);
    delete reinterpret_cast<DirectDrawSurface *>(p);
    return DD_OK;
}

HRESULT MyIDirectDrawSurface_Flip(LPDIRECTDRAWSURFACE p)
{
    assert(p);
    return reinterpret_cast<DirectDrawSurface *>(p)->Flip();
}
}

}
}
