#include "ddraw/2d_surface.h"

#include "gfx/blitter.h"
#include "gfx/context.h"
#include "gfx/screenshot.h"
#include "memory.h"

#include <string.h>

void GFX_2D_Surface_Init(
    GFX_2D_Surface *surface, GFX_2D_Renderer *renderer,
    LPDDSURFACEDESC lpDDSurfaceDesc)
{
    surface->back_buffer = NULL;
    surface->is_locked = false;
    surface->is_dirty = false;
    surface->renderer = renderer;
    surface->desc = *lpDDSurfaceDesc;

    DDSURFACEDESC displayDesc;
    displayDesc.ddpfPixelFormat.dwRGBBitCount = 32;
    displayDesc.dwWidth = GFX_Context_GetDisplayWidth();
    displayDesc.dwHeight = GFX_Context_GetDisplayHeight();
    displayDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;

    if (!(surface->desc.dwFlags & (DDSD_WIDTH | DDSD_HEIGHT))) {
        surface->desc.dwWidth = displayDesc.dwWidth;
        surface->desc.dwHeight = displayDesc.dwHeight;
        surface->desc.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
    }

    if (!(surface->desc.dwFlags & DDSD_PIXELFORMAT)) {
        surface->desc.ddpfPixelFormat = displayDesc.ddpfPixelFormat;
        surface->desc.dwFlags |= DDSD_PIXELFORMAT;
    }

    surface->desc.lPitch = surface->desc.dwWidth
        * (surface->desc.ddpfPixelFormat.dwRGBBitCount / 8);

    surface->buffer =
        Memory_Alloc(surface->desc.lPitch * surface->desc.dwHeight);
    surface->desc.lpSurface = NULL;

    if (surface->desc.dwFlags & DDSD_BACKBUFFERCOUNT
        && surface->desc.dwBackBufferCount > 0) {
        DDSURFACEDESC back_buffer_desc = surface->desc;
        back_buffer_desc.ddsCaps.dwCaps |= DDSCAPS_FLIP;
        back_buffer_desc.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
        back_buffer_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
        back_buffer_desc.dwBackBufferCount = 0;
        surface->back_buffer = Memory_Alloc(sizeof(GFX_2D_Surface));
        GFX_2D_Surface_Init(
            surface->back_buffer, surface->renderer, &back_buffer_desc);
        surface->desc.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER | DDSCAPS_FLIP;
    }
}

void GFX_2D_Surface_Close(GFX_2D_Surface *surface)
{
    if (surface->back_buffer) {
        GFX_2D_Surface_Close(surface->back_buffer);
        Memory_Free(surface->back_buffer);
        surface->back_buffer = NULL;
    }

    if (surface->buffer) {
        Memory_Free(surface->buffer);
        surface->buffer = NULL;
    }
}

HRESULT GFX_2D_Surface_Blt(
    GFX_2D_Surface *surface, LPRECT lpDestRect, GFX_2D_Surface *src,
    LPRECT lpSrcRect, DWORD dwFlags)
{
    // can't blit while locked
    if (surface->is_locked) {
        return DDERR_LOCKEDSURFACES;
    }

    if (src) {
        surface->is_dirty = true;

        int32_t dst_width = surface->desc.dwWidth;
        int32_t dst_height = surface->desc.dwHeight;

        GFX_BlitterRect dst_rect = { 0, 0, dst_width, dst_height };
        if (lpDestRect) {
            dst_rect.left = lpDestRect->left;
            dst_rect.top = lpDestRect->top;
            dst_rect.right = lpDestRect->right;
            dst_rect.bottom = lpDestRect->bottom;
        }

        int32_t depth = surface->desc.ddpfPixelFormat.dwRGBBitCount / 8;

        if (src->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
            // This is a somewhat ugly and slow hack to get a rescaled and
            // converted copy of the framebuffer for the surface, which is
            // required to display the in-game menu of Tomb Raider
            // correctly.
            GLint width;
            GLint height;
            GFX_Screenshot_CaptureToBuffer(
                NULL, &width, &height, depth, GL_BGRA,
                GL_UNSIGNED_INT_8_8_8_8_REV, false, true);
            uint8_t *buffer = Memory_Alloc(width * height * depth);
            GFX_Screenshot_CaptureToBuffer(
                buffer, &width, &height, depth, GL_BGRA,
                GL_UNSIGNED_INT_8_8_8_8_REV, false, true);

            for (int i = 0; i < width * height * depth; i++) {
                buffer[i] /= 2;
            }

            GFX_BlitterRect src_rect = { 0, height, width, 0 };
            GFX_BlitterImage src_img = { width, height, depth, buffer };
            GFX_BlitterImage dst_img = { dst_width, dst_height, depth,
                                         surface->buffer };
            GFX_Blit(&src_img, &src_rect, &dst_img, &dst_rect);
            Memory_Free(buffer);
        } else {
            int32_t src_width = src->desc.dwWidth;
            int32_t src_height = src->desc.dwHeight;

            GFX_BlitterRect src_rect = { 0, 0, src_width, src_height };
            if (lpSrcRect) {
                src_rect.left = lpSrcRect->left;
                src_rect.top = lpSrcRect->top;
                src_rect.right = lpSrcRect->right;
                src_rect.bottom = lpSrcRect->bottom;
            }

            GFX_BlitterImage src_img = { src_width, src_height, depth,
                                         src->buffer };
            GFX_BlitterImage dst_img = { dst_width, dst_height, depth,
                                         surface->buffer };

            GFX_Blit(&src_img, &src_rect, &dst_img, &dst_rect);
        }
    }

    if (dwFlags & DDBLT_COLORFILL) {
        surface->is_dirty = true;
        memset(
            surface->buffer, 0, surface->desc.lPitch * surface->desc.dwHeight);
    }

    return DD_OK;
}

HRESULT GFX_2D_Surface_Flip(GFX_2D_Surface *surface)
{
    // check if the surface can be flipped
    if (!(surface->desc.ddsCaps.dwCaps & DDSCAPS_FLIP)
        || !(surface->desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        || !surface->back_buffer) {
        return DDERR_NOTFLIPPABLE;
    }

    bool rendered = GFX_Context_IsRendered();

    // don't re-upload surfaces if external rendering was active after
    // lock() has been called, since it wouldn't be visible anyway
    if (rendered) {
        surface->is_dirty = false;
    }

    // swap front and back buffers
    uint8_t *buffer_tmp = surface->back_buffer->buffer;
    surface->back_buffer->buffer = surface->buffer;
    surface->buffer = buffer_tmp;

    bool dirty_tmp = surface->is_dirty;
    surface->is_dirty = surface->back_buffer->is_dirty;
    surface->back_buffer->is_dirty = dirty_tmp;

    // upload surface if dirty
    if (surface->is_dirty) {
        GFX_2D_Renderer_Upload(
            surface->renderer, &surface->desc, surface->buffer);
        surface->is_dirty = false;
    }

    // swap buffer now if there was external rendering, otherwise the
    // surface would overwrite it
    if (rendered) {
        GFX_Context_SwapBuffers();
    }

    // update viewport in case the window size has changed
    GFX_Context_SetupViewport();

    // render surface
    GFX_2D_Renderer_Render(surface->renderer);

    // swap buffer after the surface has been rendered if there was no
    // external rendering for this frame, fixes title screens and other pure
    // 2D operations that aren't continuously updated
    if (!rendered) {
        GFX_Context_SwapBuffers();
    }

    return DD_OK;
}

GFX_2D_Surface *GFX_2D_Surface_GetAttachedSurface(GFX_2D_Surface *surface)
{
    return surface->back_buffer;
}

HRESULT GFX_2D_Surface_GetPixelFormat(
    GFX_2D_Surface *surface, LPDDPIXELFORMAT lpDDPixelFormat)
{
    *lpDDPixelFormat = surface->desc.ddpfPixelFormat;

    return DD_OK;
}

HRESULT GFX_2D_Surface_GetSurfaceDesc(
    GFX_2D_Surface *surface, LPDDSURFACEDESC lpDDSurfaceDesc)
{
    *lpDDSurfaceDesc = surface->desc;

    return DD_OK;
}

HRESULT GFX_2D_Surface_Lock(
    GFX_2D_Surface *surface, LPDDSURFACEDESC lpDDSurfaceDesc)
{
    if (surface->is_locked) {
        return DDERR_SURFACEBUSY;
    }

    // assign lpSurface
    surface->desc.lpSurface = surface->buffer;

    surface->is_locked = true;
    surface->is_dirty = true;

    *lpDDSurfaceDesc = surface->desc;

    return DD_OK;
}

HRESULT GFX_2D_Surface_Unlock(GFX_2D_Surface *surface, LPVOID lp)
{
    // ensure that the surface is actually locked
    if (!surface->is_locked) {
        return DDERR_NOTLOCKED;
    }

    // unassign lpSurface
    surface->desc.lpSurface = NULL;

    surface->is_locked = false;

    return DD_OK;
}
