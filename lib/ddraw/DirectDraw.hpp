#pragma once

#include "Renderer.hpp"
#include "Unknown.hpp"
#include "ddraw.hpp"

#include <glrage/GLRage.hpp>

#include <cstdint>
#include <memory>

namespace glrage {
namespace ddraw {

class DirectDraw
    : public Unknown
    , public IDirectDraw
    , public IDirectDraw2
{
public:
    DirectDraw();
    virtual ~DirectDraw();

    /*** IUnknown methods ***/
    virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual ULONG WINAPI AddRef();
    virtual ULONG WINAPI Release();

    /*** IDirectDraw methods ***/
    HRESULT WINAPI Compact();
    HRESULT WINAPI CreateClipper(DWORD dwFlags,
        LPDIRECTDRAWCLIPPER* lplpDDClipper,
        IUnknown* pUnkOuter);
    HRESULT WINAPI CreatePalette(DWORD dwFlags,
        LPPALETTEENTRY lpDDColorArray,
        LPDIRECTDRAWPALETTE* lplpDDPalette,
        IUnknown* pUnkOuter);
    HRESULT WINAPI CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc,
        LPDIRECTDRAWSURFACE* lplpDDSurface,
        IUnknown* pUnkOuter);
    HRESULT WINAPI DuplicateSurface(LPDIRECTDRAWSURFACE lpDDSurface,
        LPDIRECTDRAWSURFACE* lplpDupDDSurface);
    HRESULT WINAPI EnumDisplayModes(DWORD dwFlags,
        LPDDSURFACEDESC lpDDSurfaceDesc,
        LPVOID lpContext,
        LPDDENUMMODESCALLBACK lpEnumModesCallback);
    HRESULT WINAPI EnumSurfaces(DWORD dwFlags,
        LPDDSURFACEDESC lpDDSurfaceDesc,
        LPVOID lpContext,
        LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
    HRESULT WINAPI FlipToGDISurface();
    HRESULT WINAPI GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
    HRESULT WINAPI GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc);
    HRESULT WINAPI GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes);
    HRESULT WINAPI GetGDISurface(LPDIRECTDRAWSURFACE* lplpGDIDDSSurface);
    HRESULT WINAPI GetMonitorFrequency(LPDWORD lpdwFrequency);
    HRESULT WINAPI GetScanLine(LPDWORD lpdwScanLine);
    HRESULT WINAPI GetVerticalBlankStatus(LPBOOL lpbIsInVB);
    HRESULT WINAPI Initialize(GUID* lpGUID);
    HRESULT WINAPI RestoreDisplayMode();
    HRESULT WINAPI SetCooperativeLevel(HWND hWnd, DWORD dwFlags);
    HRESULT WINAPI SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP);
    HRESULT WINAPI WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent);

    /*** IDirectDraw2 methods ***/
    HRESULT WINAPI SetDisplayMode(DWORD dwWidth,
        DWORD dwHeight,
        DWORD dwBPP,
        DWORD dwRefreshRate,
        DWORD dwFlags); // updated in v2
    HRESULT WINAPI GetAvailableVidMem(LPDDSCAPS lpDDSCaps,
        LPDWORD lpdwTotal,
        LPDWORD lpdwFree); // added in v2

private:
    const uint32_t DEFAULT_WIDTH = 640;
    const uint32_t DEFAULT_HEIGHT = 480;
    const uint32_t DEFAULT_BITS = 16;
    const uint32_t DEFAULT_REFRESH_RATE = 60;

    Context& m_context = GLRage::getContext();
    std::unique_ptr<Renderer> m_renderer = nullptr;
    uint32_t m_width = DEFAULT_WIDTH;
    uint32_t m_height = DEFAULT_HEIGHT;
    uint32_t m_refreshRate = DEFAULT_REFRESH_RATE;
    uint32_t m_bits = DEFAULT_BITS;
};

} // namespace ddraw
} // namespace glrage
