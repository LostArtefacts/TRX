#pragma once

#include "DirectDraw.hpp"
#include "DirectDrawClipper.hpp"
#include "Renderer.hpp"
#include "Unknown.hpp"
#include "ddraw.hpp"

#include <glrage/GLRage.hpp>

#include <cstdint>
#include <vector>

namespace glrage {
namespace ddraw {

class DirectDrawSurface
    : public Unknown
    , public IDirectDrawSurface
    , public IDirectDrawSurface2
{
public:
    DirectDrawSurface(DirectDraw& lpDD,
        Renderer& renderer,
        LPDDSURFACEDESC lpDDSurfaceDesc);
    virtual ~DirectDrawSurface();

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual ULONG WINAPI AddRef();
    virtual ULONG WINAPI Release();

    /*** IDirectDrawSurface methods ***/
    HRESULT WINAPI AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
    HRESULT WINAPI AddOverlayDirtyRect(LPRECT lpRect);
    HRESULT WINAPI Blt(LPRECT lpDestRect,
        LPDIRECTDRAWSURFACE lpDDSrcSurface,
        LPRECT lpSrcRect,
        DWORD dwFlags,
        LPDDBLTFX lpDDBltFx);
    HRESULT WINAPI BltBatch(LPDDBLTBATCH lpDDBltBatch,
        DWORD dwCount,
        DWORD dwFlags);
    HRESULT WINAPI BltFast(DWORD dwX,
        DWORD dwY,
        LPDIRECTDRAWSURFACE lpDDSrcSurface,
        LPRECT lpSrcRect,
        DWORD dwTrans);
    HRESULT WINAPI DeleteAttachedSurface(DWORD dwFlags,
        LPDIRECTDRAWSURFACE lpDDSurface);
    HRESULT WINAPI EnumAttachedSurfaces(LPVOID lpContext,
        LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
    HRESULT WINAPI EnumOverlayZOrders(DWORD dwFlags,
        LPVOID lpContext,
        LPDDENUMSURFACESCALLBACK lpfnCallback);
    HRESULT WINAPI Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride,
        DWORD dwFlags);
    HRESULT WINAPI GetAttachedSurface(LPDDSCAPS lpDDSCaps,
        LPDIRECTDRAWSURFACE* lplpDDAttachedSurface);
    HRESULT WINAPI GetBltStatus(DWORD dwFlags);
    HRESULT WINAPI GetCaps(LPDDSCAPS lpDDSCaps);
    HRESULT WINAPI GetClipper(LPDIRECTDRAWCLIPPER* lplpDDClipper);
    HRESULT WINAPI GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
    HRESULT WINAPI GetDC(HDC* phDC);
    HRESULT WINAPI GetFlipStatus(DWORD dwFlags);
    HRESULT WINAPI GetOverlayPosition(LPLONG lplX, LPLONG lplY);
    HRESULT WINAPI GetPalette(LPDIRECTDRAWPALETTE* lplpDDPalette);
    HRESULT WINAPI GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
    HRESULT WINAPI GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc);
    HRESULT WINAPI Initialize(LPDIRECTDRAW lpDD,
        LPDDSURFACEDESC lpDDSurfaceDesc);
    HRESULT WINAPI IsLost();
    HRESULT WINAPI Lock(LPRECT lpDestRect,
        LPDDSURFACEDESC lpDDSurfaceDesc,
        DWORD dwFlags,
        HANDLE hEvent);
    HRESULT WINAPI ReleaseDC(HDC hDC);
    HRESULT WINAPI Restore();
    HRESULT WINAPI SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper);
    HRESULT WINAPI SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
    HRESULT WINAPI SetOverlayPosition(LONG lX, LONG lY);
    HRESULT WINAPI SetPalette(LPDIRECTDRAWPALETTE lpDDPalette);
    HRESULT WINAPI Unlock(LPVOID lp);
    HRESULT WINAPI UpdateOverlay(LPRECT lpSrcRect,
        LPDIRECTDRAWSURFACE lpDDDestSurface,
        LPRECT lpDestRect,
        DWORD dwFlags,
        LPDDOVERLAYFX lpDDOverlayFx);
    HRESULT WINAPI UpdateOverlayDisplay(DWORD dwFlags);
    HRESULT WINAPI UpdateOverlayZOrder(DWORD dwFlags,
        LPDIRECTDRAWSURFACE lpDDSReference);

    /*** IDirectDrawSurface2 methods ***/
    HRESULT WINAPI AddAttachedSurface(
        LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface); // updated in v2
    HRESULT WINAPI Blt(LPRECT lpDestRect,
        LPDIRECTDRAWSURFACE2 lpDDSrcSurface,
        LPRECT lpSrcRect,
        DWORD dwFlags,
        LPDDBLTFX lpDDBltFx); // updated in v2
    HRESULT WINAPI BltFast(DWORD dwX,
        DWORD dwY,
        LPDIRECTDRAWSURFACE2 lpDDSrcSurface,
        LPRECT lpSrcRect,
        DWORD dwTrans); // updated in v2
    HRESULT WINAPI DeleteAttachedSurface(DWORD dwFlags,
        LPDIRECTDRAWSURFACE2 lpDDSurface); // updated in v2
    HRESULT WINAPI Flip(LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride,
        DWORD dwFlags); // updated in v2
    HRESULT WINAPI GetAttachedSurface(LPDDSCAPS lpDDSCaps,
        LPDIRECTDRAWSURFACE2* lplpDDAttachedSurface); // updated in v2
    HRESULT WINAPI UpdateOverlay(LPRECT lpSrcRect,
        LPDIRECTDRAWSURFACE2 lpDDDestSurface,
        LPRECT lpDestRect,
        DWORD dwFlags,
        LPDDOVERLAYFX lpDDOverlayFx); // updated in v2
    HRESULT WINAPI UpdateOverlayZOrder(DWORD dwFlags,
        LPDIRECTDRAWSURFACE2 lpDDSReference);      // updated in v2
    HRESULT WINAPI GetDDInterface(LPVOID* lplpDD); // added in v2
    HRESULT WINAPI PageLock(DWORD dwFlags);        // added in v2
    HRESULT WINAPI PageUnlock(DWORD dwFlags);      // added in v2

private:
    Context& m_context = GLRage::getContext();
    DirectDraw& m_dd;
    Renderer& m_renderer;
    std::vector<uint8_t> m_buffer;
    DDSURFACEDESC m_desc;
    DirectDrawSurface* m_backBuffer = nullptr;
    DirectDrawSurface* m_depthBuffer = nullptr;
    DirectDrawClipper* m_clipper = nullptr;
    bool m_locked = false;
    bool m_dirty = false;

    /*** Custom methods ***/
    void clear(int32_t color);
    void rgba5551AdjustBrightness(bool brighten);
};

} // namespace ddraw
} // namespace glrage
