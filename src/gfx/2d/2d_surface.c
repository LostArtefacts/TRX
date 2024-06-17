#include "gfx/2d/2d_surface.h"

#include "gfx/blitter.h"
#include "gfx/context.h"

#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <assert.h>
#include <string.h>

GFX_2D_Surface *GFX_2D_Surface_Create(const GFX_2D_SurfaceDesc *desc)
{
    GFX_2D_Surface *surface = Memory_Alloc(sizeof(GFX_2D_Surface));
    GFX_2D_Surface_Init(surface, desc);
    return surface;
}

void GFX_2D_Surface_Free(GFX_2D_Surface *surface)
{
    if (surface) {
        GFX_2D_Surface_Close(surface);
        Memory_FreePointer(&surface);
    }
}

void GFX_2D_Surface_Init(
    GFX_2D_Surface *surface, const GFX_2D_SurfaceDesc *desc)
{
    surface->is_locked = false;
    surface->is_dirty = false;
    surface->desc = *desc;

    GFX_2D_SurfaceDesc display_desc = {
        .bit_count = 32,
        .width = GFX_Context_GetDisplayWidth(),
        .height = GFX_Context_GetDisplayHeight(),
    };

    if (!surface->desc.width || !surface->desc.height) {
        surface->desc.width = display_desc.width;
        surface->desc.height = display_desc.height;
    }

    if (!surface->desc.bit_count) {
        surface->desc.bit_count = display_desc.bit_count;
    }

    surface->desc.pitch = surface->desc.width * (surface->desc.bit_count / 8);

    surface->buffer = Memory_Alloc(surface->desc.pitch * surface->desc.height);
    surface->desc.pixels = NULL;
}

void GFX_2D_Surface_Close(GFX_2D_Surface *surface)
{
    Memory_FreePointer(&surface->buffer);
}

bool GFX_2D_Surface_Clear(GFX_2D_Surface *surface)
{
    if (surface->is_locked) {
        LOG_ERROR("Surface is locked");
        return false;
    }

    surface->is_dirty = true;
    memset(surface->buffer, 0, surface->desc.pitch * surface->desc.height);
    return true;
}

bool GFX_2D_Surface_Blt(
    GFX_2D_Surface *surface, const GFX_BlitterRect *dst_rect,
    GFX_2D_Surface *src, const GFX_BlitterRect *src_rect)
{
    if (surface->is_locked) {
        LOG_ERROR("Surface is locked");
        return false;
    }

    if (src) {
        surface->is_dirty = true;

        int32_t dst_width = surface->desc.width;
        int32_t dst_height = surface->desc.height;
        const GFX_BlitterRect default_dst_rect = { 0, 0, dst_width,
                                                   dst_height };

        int32_t depth = surface->desc.bit_count / 8;

        int32_t src_width = src->desc.width;
        int32_t src_height = src->desc.height;
        const GFX_BlitterRect default_src_rect = {
            .left = 0, .top = 0, .right = src_width, .bottom = src_height
        };

        GFX_BlitterImage src_img = { src_width, src_height, depth,
                                     src->buffer };
        GFX_BlitterImage dst_img = { dst_width, dst_height, depth,
                                     surface->buffer };

        GFX_Blit(
            &src_img, src_rect ? src_rect : &default_src_rect, &dst_img,
            dst_rect ? dst_rect : &default_dst_rect);
    }

    return true;
}

bool GFX_2D_Surface_Lock(GFX_2D_Surface *surface, GFX_2D_SurfaceDesc *out_desc)
{
    assert(surface != NULL);
    if (surface->is_locked) {
        LOG_ERROR("Surface is busy");
        return false;
    }

    // assign pixels
    surface->desc.pixels = surface->buffer;

    surface->is_locked = true;
    surface->is_dirty = true;

    *out_desc = surface->desc;

    return true;
}

bool GFX_2D_Surface_Unlock(GFX_2D_Surface *surface)
{
    // ensure that the surface is actually locked
    if (!surface->is_locked) {
        LOG_ERROR("Surface is not locked");
        return false;
    }

    // unassign pixels
    surface->desc.pixels = NULL;

    surface->is_locked = false;

    return true;
}
