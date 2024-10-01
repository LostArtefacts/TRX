#include "gfx/2d/2d_surface.h"

#include "gfx/context.h"
#include "log.h"
#include "memory.h"

#include <assert.h>
#include <string.h>

GFX_2D_SURFACE *GFX_2D_Surface_Create(const GFX_2D_SURFACE_DESC *desc)
{
    GFX_2D_SURFACE *surface = Memory_Alloc(sizeof(GFX_2D_SURFACE));
    GFX_2D_Surface_Init(surface, desc);
    return surface;
}

GFX_2D_SURFACE *GFX_2D_Surface_CreateFromImage(const IMAGE *image)
{
    GFX_2D_SURFACE *surface = Memory_Alloc(sizeof(GFX_2D_SURFACE));
    surface->is_locked = false;
    surface->is_dirty = true;
    surface->desc.width = image->width;
    surface->desc.height = image->height;
    surface->desc.bit_count = 24;
    surface->desc.tex_format = GL_RGB;
    surface->desc.tex_type = GL_UNSIGNED_BYTE;
    surface->desc.pitch = surface->desc.width * (surface->desc.bit_count / 8);
    surface->desc.pixels = NULL;
    surface->buffer = Memory_Alloc(surface->desc.pitch * surface->desc.height);
    memcpy(
        surface->buffer, image->data,
        surface->desc.pitch * surface->desc.height);
    return surface;
}

void GFX_2D_Surface_Free(GFX_2D_SURFACE *surface)
{
    if (surface) {
        GFX_2D_Surface_Close(surface);
        Memory_FreePointer(&surface);
    }
}

void GFX_2D_Surface_Init(
    GFX_2D_SURFACE *surface, const GFX_2D_SURFACE_DESC *desc)
{
    surface->is_locked = false;
    surface->is_dirty = false;
    surface->desc = *desc;

    GFX_2D_SURFACE_DESC display_desc = {
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

    if (!surface->desc.tex_format) {
        surface->desc.tex_format = GL_BGRA;
    }
    if (!surface->desc.tex_type) {
        surface->desc.tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    surface->desc.pitch = surface->desc.width * (surface->desc.bit_count / 8);

    surface->buffer = Memory_Alloc(surface->desc.pitch * surface->desc.height);
    surface->desc.pixels = NULL;
}

void GFX_2D_Surface_Close(GFX_2D_SURFACE *surface)
{
    Memory_FreePointer(&surface->buffer);
}

bool GFX_2D_Surface_Clear(GFX_2D_SURFACE *surface)
{
    if (surface->is_locked) {
        LOG_ERROR("Surface is locked");
        return false;
    }

    surface->is_dirty = true;
    memset(surface->buffer, 0, surface->desc.pitch * surface->desc.height);
    return true;
}

bool GFX_2D_Surface_Lock(GFX_2D_SURFACE *surface, GFX_2D_SURFACE_DESC *out_desc)
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

bool GFX_2D_Surface_Unlock(GFX_2D_SURFACE *surface)
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
