#include "ddraw/DirectDrawSurface.hpp"

#include "ddraw/Blitter.hpp"
#include "glrage_gl/Screenshot.hpp"

#include <algorithm>

namespace glrage {
namespace ddraw {

DirectDrawSurface::DirectDrawSurface(
    DirectDraw &lpDD, Renderer &renderer, LPDDSURFACEDESC lpDDSurfaceDesc)
    : m_renderer(renderer)
    , m_desc(*lpDDSurfaceDesc)
{
    DDSURFACEDESC displayDesc;
    displayDesc.ddpfPixelFormat.dwRGBBitCount = 32;
    displayDesc.dwWidth = m_context.getDisplayWidth();
    displayDesc.dwHeight = m_context.getDisplayHeight();
    displayDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;

    if (!(m_desc.dwFlags & (DDSD_WIDTH | DDSD_HEIGHT))) {
        m_desc.dwWidth = displayDesc.dwWidth;
        m_desc.dwHeight = displayDesc.dwHeight;
        m_desc.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
    }

    if (!(m_desc.dwFlags & DDSD_PIXELFORMAT)) {
        m_desc.ddpfPixelFormat = displayDesc.ddpfPixelFormat;
        m_desc.dwFlags |= DDSD_PIXELFORMAT;
    }

    m_desc.lPitch = m_desc.dwWidth * (m_desc.ddpfPixelFormat.dwRGBBitCount / 8);

    m_buffer.resize(m_desc.lPitch * m_desc.dwHeight, 0);
    m_desc.lpSurface = nullptr;

    if (m_desc.dwFlags & DDSD_BACKBUFFERCOUNT && m_desc.dwBackBufferCount > 0) {
        DDSURFACEDESC backBufferDesc = m_desc;
        backBufferDesc.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER | DDSCAPS_FLIP;
        backBufferDesc.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
        backBufferDesc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
        backBufferDesc.dwBackBufferCount = 0;
        m_backBuffer = std::make_unique<DirectDrawSurface>(
            lpDD, m_renderer, &backBufferDesc);
        m_desc.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER | DDSCAPS_FLIP;
    }
}

HRESULT DirectDrawSurface::Blt(
    LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect,
    DWORD dwFlags)
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
                GL_UNSIGNED_INT_8_8_8_8_REV, false);

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
        m_dirty = true;
        std::fill(m_buffer.begin(), m_buffer.end(), 0);
    }

    return DD_OK;
}

HRESULT DirectDrawSurface::Flip()
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

HRESULT DirectDrawSurface::GetAttachedSurface(
    LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE *lplpDDAttachedSurface)
{
    if (lpDDSCaps->dwCaps & DDSCAPS_BACKBUFFER) {
        *lplpDDAttachedSurface = m_backBuffer.get();
        return DD_OK;
    }

    return DDERR_SURFACENOTATTACHED;
}

HRESULT
DirectDrawSurface::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
    *lpDDPixelFormat = m_desc.ddpfPixelFormat;

    return DD_OK;
}

HRESULT
DirectDrawSurface::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
    *lpDDSurfaceDesc = m_desc;

    return DD_OK;
}

HRESULT DirectDrawSurface::Lock(LPDDSURFACEDESC lpDDSurfaceDesc)
{
    if (m_locked) {
        return DDERR_SURFACEBUSY;
    }

    // assign lpSurface
    m_desc.lpSurface = &m_buffer[0];

    m_locked = true;
    m_dirty = true;

    *lpDDSurfaceDesc = m_desc;

    return DD_OK;
}

HRESULT DirectDrawSurface::Unlock(LPVOID lp)
{
    // ensure that the surface is actually locked
    if (!m_locked) {
        return DDERR_NOTLOCKED;
    }

    // unassign lpSurface
    m_desc.lpSurface = nullptr;

    m_locked = false;

    return DD_OK;
}

}
}
