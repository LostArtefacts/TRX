#include "DirectDraw.hpp"
#include "DirectDrawClipper.hpp"
#include "DirectDrawSurface.hpp"

#include <glrage_util/Logger.hpp>

namespace glrage {
namespace ddraw {

DirectDraw::DirectDraw()
{}

DirectDraw::~DirectDraw()
{}

/*** IUnknown methods ***/
HRESULT WINAPI DirectDraw::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualGUID(riid, IID_IDirectDraw)) {
        *ppvObj = static_cast<IDirectDraw*>(this);
    } else if (IsEqualGUID(riid, IID_IDirectDraw2)) {
        *ppvObj = static_cast<IDirectDraw2*>(this);
    } else {
        return Unknown::QueryInterface(riid, ppvObj);
    }

    Unknown::AddRef();
    return DD_OK;
}

ULONG WINAPI DirectDraw::AddRef()
{
    return Unknown::AddRef();
}

ULONG WINAPI DirectDraw::Release()
{
    m_renderer.reset();
    return Unknown::Release();
}

/*** IDirectDraw methods ***/
HRESULT WINAPI DirectDraw::Compact()
{
    return DD_OK;
}

HRESULT WINAPI DirectDraw::CreateClipper(DWORD dwFlags,
    LPDIRECTDRAWCLIPPER* lplpDDClipper,
    IUnknown* pUnkOuter)
{
    *lplpDDClipper = new DirectDrawClipper();

    return DD_OK;
}

HRESULT WINAPI DirectDraw::CreatePalette(DWORD dwFlags,
    LPPALETTEENTRY lpDDColorArray,
    LPDIRECTDRAWPALETTE* lplpDDPalette,
    IUnknown* pUnkOuter)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDraw::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc,
    LPDIRECTDRAWSURFACE* lplpDDSurface,
    IUnknown* pUnkOuter)
{
    *lplpDDSurface = new DirectDrawSurface(*this, *m_renderer, lpDDSurfaceDesc);

    return DD_OK;
}

HRESULT WINAPI DirectDraw::DuplicateSurface(LPDIRECTDRAWSURFACE lpDDSurface,
    LPDIRECTDRAWSURFACE* lplpDupDDSurface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDraw::EnumDisplayModes(DWORD dwFlags,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    LPVOID lpContext,
    LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
    if (lpDDSurfaceDesc) {
        // just give what the app wants
        lpEnumModesCallback(lpDDSurfaceDesc, lpContext);
    } else {
        DDSURFACEDESC desc;
        desc.dwSize = sizeof(DDSURFACEDESC);
        desc.dwWidth = DEFAULT_WIDTH;
        desc.dwHeight = DEFAULT_HEIGHT;
        desc.lPitch = desc.dwWidth * 2;

        lpEnumModesCallback(&desc, lpContext);
    }

    return DD_OK;
}

HRESULT WINAPI DirectDraw::EnumSurfaces(DWORD dwFlags,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    LPVOID lpContext,
    LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDraw::FlipToGDISurface()
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDraw::GetCaps(LPDDCAPS lpDDDriverCaps,
    LPDDCAPS lpDDHELCaps)
{
    LPDDCAPS caps[2] = {lpDDDriverCaps, lpDDHELCaps};
    for (uint32_t i = 0; i < 2; i++) {
        auto cap = caps[i];
        if (!cap) {
            continue;
        }

        cap->dwZBufferBitDepths = DDBD_16 | DDBD_32;
        cap->dwVidMemTotal = cap->dwVidMemFree = 4 << 20;
        cap->ddsCaps.dwCaps =
            DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER;
    }

    return DD_OK;
}

HRESULT WINAPI DirectDraw::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc)
{
    LPDDSURFACEDESC desc = lpDDSurfaceDesc;
    desc->ddpfPixelFormat.dwFlags = DDPF_RGB;
    desc->ddpfPixelFormat.dwRGBBitCount = 32;
    desc->dwWidth = m_width;
    desc->dwHeight = m_height;
    desc->lPitch = m_width * (desc->ddpfPixelFormat.dwRGBBitCount / 8);
    desc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT;
    return DD_OK;
}

HRESULT WINAPI DirectDraw::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDraw::GetGDISurface(LPDIRECTDRAWSURFACE* lplpGDIDDSSurface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDraw::GetMonitorFrequency(LPDWORD lpdwFrequency)
{
    if (lpdwFrequency) {
        *lpdwFrequency = m_refreshRate;
    }

    return DD_OK;
}

HRESULT WINAPI DirectDraw::GetScanLine(LPDWORD lpdwScanLine)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDraw::GetVerticalBlankStatus(LPBOOL lpbIsInVB)
{
    if (lpbIsInVB) {
        *lpbIsInVB = TRUE;
    }

    return DD_OK;
}

HRESULT WINAPI DirectDraw::Initialize(GUID* lpGUID)
{
    m_renderer = std::make_unique<Renderer>();
    return DD_OK;
}

HRESULT WINAPI DirectDraw::RestoreDisplayMode()
{
    // nothing to do, desktop display is never touched
    return DD_OK;
}

HRESULT WINAPI DirectDraw::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
    return DD_OK;
}

HRESULT WINAPI DirectDraw::SetDisplayMode(DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwBPP)
{
    return SetDisplayMode(dwWidth, dwHeight, dwBPP, DEFAULT_REFRESH_RATE, 0);
}

HRESULT WINAPI DirectDraw::WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent)
{
    return DD_OK;
}

/*** IDirectDraw2 methods ***/
HRESULT WINAPI DirectDraw::SetDisplayMode(DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwBPP,
    DWORD dwRefreshRate,
    DWORD dwFlags)
{
    m_width = dwWidth;
    m_height = dwHeight;
    m_bits = dwBPP;
    m_refreshRate = dwRefreshRate;

    m_context.setDisplaySize(m_width, m_height);

    return DD_OK;
}

HRESULT WINAPI DirectDraw::GetAvailableVidMem(LPDDSCAPS lpDDSCaps,
    LPDWORD lpdwTotal,
    LPDWORD lpdwFree)
{
    // just return 8 MiB, which is plenty for mid-90s hardware
    *lpdwTotal = *lpdwFree = 8 << 20;

    return DD_OK;
}

} // namespace ddraw
} // namespace glrage
