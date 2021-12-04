#include "ddraw/Interop.hpp"

#include "ddraw/DirectDraw.hpp"
#include "ddraw/DirectDrawSurface.hpp"
#include "glrage/Context.hpp"
#include "glrage_util/ErrorUtils.hpp"

#include <cassert>
#include <string>

namespace glrage {
namespace ddraw {

static Context &context = Context::instance();

extern "C" {

static DirectDraw *m_DDraw = nullptr;

HRESULT MyDirectDrawCreate()
{
    try {
        m_DDraw = new DirectDraw();
    } catch (const std::exception &ex) {
        ErrorUtils::warning(ex);
        return DDERR_GENERIC;
    }

    return DD_OK;
}

HRESULT MyIDirectDraw_Release()
{
    assert(m_DDraw);
    delete m_DDraw;
    return DD_OK;
}

HRESULT MyIDirectDraw2_CreateSurface(
    LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface)
{
    return m_DDraw->CreateSurface(lpDDSurfaceDesc, lplpDDSurface);
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
