#pragma once

#include "gfx/blitter.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t width;
    int32_t height;
    int32_t pitch;
    void *pixels;
    int32_t bit_count;
} GFX_2D_SurfaceDesc;

typedef struct GFX_2D_Surface {
    uint8_t *buffer;
    GFX_2D_SurfaceDesc desc;
    bool is_locked;
    bool is_dirty;
} GFX_2D_Surface;

GFX_2D_Surface *GFX_2D_Surface_Create(const GFX_2D_SurfaceDesc *desc);
void GFX_2D_Surface_Free(GFX_2D_Surface *surface);

void GFX_2D_Surface_Init(
    GFX_2D_Surface *surface, const GFX_2D_SurfaceDesc *desc);
void GFX_2D_Surface_Close(GFX_2D_Surface *surface);

bool GFX_2D_Surface_Clear(GFX_2D_Surface *surface);
bool GFX_2D_Surface_Blt(
    GFX_2D_Surface *surface, const GFX_BlitterRect *dst_rect,
    GFX_2D_Surface *src, const GFX_BlitterRect *src_rect);

bool GFX_2D_Surface_Lock(GFX_2D_Surface *surface, GFX_2D_SurfaceDesc *out_desc);
bool GFX_2D_Surface_Unlock(GFX_2D_Surface *surface);
