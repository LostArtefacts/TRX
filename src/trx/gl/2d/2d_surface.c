#include <trx/gl/2d/2d_surface.h>

#include <trx/debug.h>
#include <trx/gl/context.h>
#include <trx/log.h>
#include <trx/memory.h>

#include <string.h>

static void M_FillDefaultUVs(TRX_GL_2D_SURFACE *const surface)
{
    surface->desc.uv[0].u = 0;
    surface->desc.uv[0].v = 0;
    surface->desc.uv[1].u = 1;
    surface->desc.uv[1].v = 0;
    surface->desc.uv[2].u = 1;
    surface->desc.uv[2].v = 1;
    surface->desc.uv[3].u = 0;
    surface->desc.uv[3].v = 1;
}

TRX_GL_2D_SURFACE *TRX_GL_2D_Surface_Create(
    const TRX_GL_2D_SURFACE_DESC *const desc)
{
    TRX_GL_2D_SURFACE *surface = Memory_Alloc(sizeof(TRX_GL_2D_SURFACE));
    TRX_GL_2D_Surface_Init(surface, desc);
    return surface;
}

TRX_GL_2D_SURFACE *TRX_GL_2D_Surface_CreateFromImage(const IMAGE *const image)
{
    TRX_GL_2D_SURFACE *surface = Memory_Alloc(sizeof(TRX_GL_2D_SURFACE));
    surface->desc.width = image->width;
    surface->desc.height = image->height;
    surface->desc.bit_count = 24;
    surface->desc.tex_format = GL_RGB;
    surface->desc.tex_type = GL_UNSIGNED_BYTE;
    surface->desc.pitch = surface->desc.width * (surface->desc.bit_count / 8);
    M_FillDefaultUVs(surface);
    surface->buffer = Memory_Alloc(surface->desc.pitch * surface->desc.height);
    memcpy(
        surface->buffer, image->data,
        surface->desc.pitch * surface->desc.height);
    return surface;
}

void TRX_GL_2D_Surface_Free(TRX_GL_2D_SURFACE *const surface)
{
    if (surface) {
        TRX_GL_2D_Surface_Close(surface);
        Memory_Free(surface);
    }
}

void TRX_GL_2D_Surface_Init(
    TRX_GL_2D_SURFACE *const surface, const TRX_GL_2D_SURFACE_DESC *const desc)
{
    surface->desc = *desc;

    TRX_GL_2D_SURFACE_DESC display_desc = {
        .bit_count = 32,
        .width = Viewport_GetWidth(VIEWPORT_GAME),
        .height = Viewport_GetHeight(VIEWPORT_GAME),
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

    bool uvs_supplied = false;
    for (int32_t i = 0; i < 4; i++) {
        uvs_supplied |=
            surface->desc.uv[i].u != 0 || surface->desc.uv[i].v != 0;
    }
    if (!uvs_supplied) {
        M_FillDefaultUVs(surface);
    }

    surface->desc.pitch = surface->desc.width * (surface->desc.bit_count / 8);

    surface->buffer = Memory_Alloc(surface->desc.pitch * surface->desc.height);
}

void TRX_GL_2D_Surface_Close(TRX_GL_2D_SURFACE *const surface)
{
    Memory_FreePointer(&surface->buffer);
}

void TRX_GL_2D_Surface_Clear(
    TRX_GL_2D_SURFACE *const surface, const uint8_t value)
{
    memset(surface->buffer, value, surface->desc.pitch * surface->desc.height);
}
