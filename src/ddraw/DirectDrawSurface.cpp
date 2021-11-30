#include "ddraw/DirectDrawSurface.hpp"

#include "ddraw/Blitter.hpp"
#include "ddraw/DirectDrawClipper.hpp"
#include "glrage_gl/Screenshot.hpp"
#include "glrage_util/Logger.hpp"

#include <algorithm>

namespace glrage {
namespace ddraw {

DirectDrawSurface::DirectDrawSurface(
    DirectDraw &lpDD, Renderer &renderer, LPDDSURFACEDESC lpDDSurfaceDesc)
    : m_dd(lpDD)
    , m_renderer(renderer)
    , m_desc(*lpDDSurfaceDesc)
{
    m_dd.AddRef();

    DDSURFACEDESC displayDesc;
    m_dd.GetDisplayMode(&displayDesc);

    // use display size if surface has no defined dimensions
    if (!(m_desc.dwFlags & (DDSD_WIDTH | DDSD_HEIGHT))) {
        m_desc.dwWidth = displayDesc.dwWidth;
        m_desc.dwHeight = displayDesc.dwHeight;
        m_desc.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
    }

    // use display pixel format if surface has no defined pixel format
    if (!(m_desc.dwFlags & DDSD_PIXELFORMAT)) {
        m_desc.ddpfPixelFormat = displayDesc.ddpfPixelFormat;
        m_desc.dwFlags |= DDSD_PIXELFORMAT;
    }

    // calculate pitch if surface has no defined pitch
    if (!(m_desc.dwFlags & DDSD_PITCH)) {
        m_desc.lPitch =
            m_desc.dwWidth * (m_desc.ddpfPixelFormat.dwRGBBitCount / 8);
        m_desc.dwFlags |= DDSD_PITCH;
    }

    // allocate surface buffer
    m_buffer.resize(m_desc.lPitch * m_desc.dwHeight, 0);
    m_desc.lpSurface = nullptr;

    // attach back buffer if defined
    if (m_desc.dwFlags & DDSD_BACKBUFFERCOUNT && m_desc.dwBackBufferCount > 0) {

        DDSURFACEDESC backBufferDesc = m_desc;
        backBufferDesc.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER | DDSCAPS_FLIP;
        backBufferDesc.ddsCaps.dwCaps &=
            ~(DDSCAPS_FRONTBUFFER | DDSCAPS_VISIBLE);
        backBufferDesc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
        backBufferDesc.dwBackBufferCount = 0;
        m_backBuffer = new DirectDrawSurface(lpDD, m_renderer, &backBufferDesc);

        m_desc.ddsCaps.dwCaps |=
            DDSCAPS_FRONTBUFFER | DDSCAPS_FLIP | DDSCAPS_VISIBLE;
    }
}

DirectDrawSurface::~DirectDrawSurface()
{
    if (m_backBuffer) {
        m_backBuffer->Release();
        m_backBuffer = nullptr;
    }

    if (m_depthBuffer) {
        m_depthBuffer->Release();
        m_depthBuffer = nullptr;
    }

    if (m_desc.lpSurface) {
        m_desc.lpSurface = nullptr;
    }
}

/*** IUnknown methods ***/
HRESULT WINAPI DirectDrawSurface::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if (IsEqualGUID(riid, IID_IDirectDrawSurface)) {
        *ppvObj = static_cast<IDirectDrawSurface *>(this);
    } else if (IsEqualGUID(riid, IID_IDirectDrawSurface2)) {
        *ppvObj = static_cast<IDirectDrawSurface2 *>(this);
    } else {
        return Unknown::QueryInterface(riid, ppvObj);
    }

    Unknown::AddRef();
    return S_OK;
}

ULONG WINAPI DirectDrawSurface::AddRef()
{
    return Unknown::AddRef();
}

ULONG WINAPI DirectDrawSurface::Release()
{
    return Unknown::Release();
}

/*** IDirectDrawSurface methods ***/
HRESULT WINAPI
DirectDrawSurface::AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
    if (!lpDDSAttachedSurface) {
        return DDERR_INVALIDOBJECT;
    }

    DirectDrawSurface *ps =
        static_cast<DirectDrawSurface *>(lpDDSAttachedSurface);
    DWORD caps = ps->m_desc.ddsCaps.dwCaps;
    if (caps & DDSCAPS_ZBUFFER) {
        m_depthBuffer = ps;
    } else if (caps & DDSCAPS_BACKBUFFER) {
        m_backBuffer = ps;
    } else {
        return DDERR_CANNOTATTACHSURFACE;
    }

    ps->AddRef();
    return DD_OK;
}

HRESULT WINAPI DirectDrawSurface::AddOverlayDirtyRect(LPRECT lpRect)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::Blt(
    LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect,
    DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
    // can't blit while locked
    if (m_locked) {
        return DDERR_LOCKEDSURFACES;
    }

    if (lpDDSrcSurface) {
        m_dirty = true;

        int32_t dstWidth = m_desc.dwWidth;
        int32_t dstHeight = m_desc.dwHeight;

        Blitter::Rect dstRect { 0, 0, dstWidth, dstHeight };
        if (lpDestRect) {
            dstRect.left = lpDestRect->left;
            dstRect.top = lpDestRect->top;
            dstRect.right = lpDestRect->right;
            dstRect.bottom = lpDestRect->bottom;
        }

        auto src = static_cast<DirectDrawSurface *>(lpDDSrcSurface);

        int32_t depth = m_desc.ddpfPixelFormat.dwRGBBitCount / 8;

        if (src->m_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
            // This is a somewhat ugly and slow hack to get a rescaled and
            // converted copy of the framebuffer for the surface, which is
            // required to display the in-game menu of Tomb Raider
            // correctly.
            GLint width;
            GLint height;
            std::vector<uint8_t> buffer;

            gl::Screenshot::capture(
                buffer, width, height, depth, GL_BGRA,
                GL_UNSIGNED_INT_8_8_8_8_REV);

            Blitter::Rect srcRect { 0, height, width, 0 };

            Blitter::Image srcImg { width, height, depth, buffer };
            // simulate dimming of DOS/PSX menu
            for (auto &pix : srcImg.buffer) {
                pix /= 2;
            }
            Blitter::Image dstImg { dstWidth, dstHeight, depth, m_buffer };

            Blitter::blit(srcImg, srcRect, dstImg, dstRect);
        } else {
            int32_t srcWidth = src->m_desc.dwWidth;
            int32_t srcHeight = src->m_desc.dwHeight;

            Blitter::Rect srcRect { 0, 0, srcWidth, srcHeight };

            if (lpSrcRect) {
                srcRect.left = lpSrcRect->left;
                srcRect.top = lpSrcRect->top;
                srcRect.right = lpSrcRect->right;
                srcRect.bottom = lpSrcRect->bottom;
            }

            Blitter::Image srcImg { srcWidth, srcHeight, depth, src->m_buffer };
            Blitter::Image dstImg { dstWidth, dstHeight, depth, m_buffer };

            Blitter::blit(srcImg, srcRect, dstImg, dstRect);
        }
    }

    if (dwFlags & DDBLT_COLORFILL) {
        clear(lpDDBltFx->dwFillColor);
    }

    if (dwFlags & DDBLT_DEPTHFILL && m_depthBuffer) {
        m_depthBuffer->clear(0);
    }

    return DD_OK;
}

HRESULT WINAPI DirectDrawSurface::BltBatch(
    LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
    // can't blit while locked
    if (m_locked) {
        return DDERR_LOCKEDSURFACES;
    }

    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::BltFast(
    DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect,
    DWORD dwTrans)
{
    // can't blit while locked
    if (m_locked) {
        return DDERR_LOCKEDSURFACES;
    }

    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::DeleteAttachedSurface(
    DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSurface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::EnumAttachedSurfaces(
    LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::EnumOverlayZOrders(
    DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::Flip(
    LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags)
{
    // check if the surface can be flipped
    if (!(m_desc.ddsCaps.dwCaps & DDSCAPS_FLIP)
        || !(m_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) || !m_backBuffer) {
        return DDERR_NOTFLIPPABLE;
    }

    bool rendered = m_context.isRendered();

    // don't re-upload surfaces if external rendering was active after
    // lock() has been called, since it wouldn't be visible anyway
    if (rendered) {
        m_dirty = false;
    }

    // swap front and back buffers
    // TODO: use buffer chain correctly
    // TODO: use lpDDSurfaceTargetOverride when defined
    m_buffer.swap(m_backBuffer->m_buffer);

    bool dirtyTmp = m_dirty;
    m_dirty = m_backBuffer->m_dirty;
    m_backBuffer->m_dirty = dirtyTmp;

    // upload surface if dirty
    if (m_dirty) {
        m_renderer.upload(m_desc, m_buffer);
        m_dirty = false;
    }

    // swap buffer now if there was external rendering, otherwise the
    // surface would overwrite it
    if (rendered) {
        m_context.swapBuffers();
    }

    // update viewport in case the window size has changed
    m_context.setupViewport();

    // render surface
    m_renderer.render();

    // swap buffer after the surface has been rendered if there was no
    // external rendering for this frame, fixes title screens and other pure
    // 2D operations that aren't continuously updated
    if (!rendered) {
        m_context.swapBuffers();
    }

    return DD_OK;
}

HRESULT WINAPI DirectDrawSurface::GetAttachedSurface(
    LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE *lplpDDAttachedSurface)
{
    if (lpDDSCaps->dwCaps & DDSCAPS_BACKBUFFER) {
        *lplpDDAttachedSurface = m_backBuffer;
        return DD_OK;
    }

    if (lpDDSCaps->dwCaps & DDSCAPS_ZBUFFER) {
        *lplpDDAttachedSurface = m_depthBuffer;
        return DD_OK;
    }

    return DDERR_SURFACENOTATTACHED;
}

HRESULT WINAPI DirectDrawSurface::GetBltStatus(DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::GetCaps(LPDDSCAPS lpDDSCaps)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::GetClipper(LPDIRECTDRAWCLIPPER *lplpDDClipper)
{
    *lplpDDClipper = reinterpret_cast<LPDIRECTDRAWCLIPPER>(m_clipper);

    return DD_OK;
}

HRESULT WINAPI
DirectDrawSurface::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::GetDC(HDC *phDC)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::GetFlipStatus(DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::GetPalette(LPDIRECTDRAWPALETTE *lplpDDPalette)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
DirectDrawSurface::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
    *lpDDPixelFormat = m_desc.ddpfPixelFormat;

    return DD_OK;
}

HRESULT WINAPI
DirectDrawSurface::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
    *lpDDSurfaceDesc = m_desc;

    return DD_OK;
}

HRESULT WINAPI DirectDrawSurface::Initialize(
    LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
    // "This method is provided for compliance with the Component Object
    // Model (COM). Because the DirectDrawSurface object is initialized when
    // it is created, this method always returns DDERR_ALREADYINITIALIZED."
    return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI DirectDrawSurface::IsLost()
{
    // we're never lost..
    return DD_OK;
}

HRESULT WINAPI DirectDrawSurface::Lock(
    LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags,
    HANDLE hEvent)
{
    // ensure that the surface is not already locked
    if (m_locked) {
        return DDERR_SURFACEBUSY;
    }

    // assign lpSurface
    m_desc.lpSurface = &m_buffer[0];
    m_desc.dwFlags |= DDSD_LPSURFACE;

    m_locked = true;
    m_dirty = true;

    *lpDDSurfaceDesc = m_desc;

    return DD_OK;
}

HRESULT WINAPI DirectDrawSurface::ReleaseDC(HDC hDC)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::Restore()
{
    // we can't lose surfaces..
    return DD_OK;
}

HRESULT WINAPI DirectDrawSurface::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
    m_clipper = reinterpret_cast<DirectDrawClipper *>(lpDDClipper);

    return DD_OK;
}

HRESULT WINAPI
DirectDrawSurface::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::SetOverlayPosition(LONG lX, LONG lY)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::Unlock(LPVOID lp)
{
    // ensure that the surface is actually locked
    if (!m_locked) {
        return DDERR_NOTLOCKED;
    }

    // unassign lpSurface
    m_desc.lpSurface = nullptr;
    m_desc.dwFlags &= ~DDSD_LPSURFACE;

    m_locked = false;

    // re-draw stand-alone back buffers immediately after unlocking
    // (used for video sequences)
    if (m_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE
        && !(m_desc.ddsCaps.dwCaps & DDSCAPS_FLIP)) {

        // FMV hack for Tomb Raider
        // fix black lines by copying even to odd lines
        for (DWORD i = 0; i < m_desc.dwHeight; i += 2) {
            auto itrEven = std::next(m_buffer.begin(), i * m_desc.lPitch);
            auto itrOdd = std::next(m_buffer.begin(), (i + 1) * m_desc.lPitch);
            std::copy(itrEven, std::next(itrEven, m_desc.lPitch), itrOdd);
        }

        m_context.swapBuffers();
        m_context.setupViewport();
        m_renderer.upload(m_desc, m_buffer);
        m_renderer.render();
    }

    return DD_OK;
}

HRESULT WINAPI DirectDrawSurface::UpdateOverlay(
    LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect,
    DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::UpdateOverlayDisplay(DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::UpdateOverlayZOrder(
    DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference)
{
    return DDERR_UNSUPPORTED;
}

/*** IDirectDrawSurface2 methods ***/
HRESULT WINAPI
DirectDrawSurface::AddAttachedSurface(LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::Blt(
    LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect,
    DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::BltFast(
    DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect,
    DWORD dwTrans)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::DeleteAttachedSurface(
    DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSurface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::Flip(
    LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::GetAttachedSurface(
    LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 *lplpDDAttachedSurface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::UpdateOverlay(
    LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect,
    DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::UpdateOverlayZOrder(
    DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::GetDDInterface(LPVOID *lplpDD)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::PageLock(DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI DirectDrawSurface::PageUnlock(DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

/*** Custom methods ***/
void DirectDrawSurface::clear(int32_t color)
{
    if (m_desc.ddpfPixelFormat.dwRGBBitCount == 8 || color == 0) {
        std::fill(m_buffer.begin(), m_buffer.end(), color & 0xff);
    } else if (m_desc.ddpfPixelFormat.dwRGBBitCount % 8 == 0) {
        int32_t i = 0;
        std::generate(m_buffer.begin(), m_buffer.end(), [this, &i, &color]() {
            int32_t colorOffset =
                i++ * 8 % this->m_desc.ddpfPixelFormat.dwRGBBitCount;
            return (color >> colorOffset) & 0xff;
        });
    } else {
        // TODO: support odd bit counts?
    }

    m_dirty = true;
}

}
}
