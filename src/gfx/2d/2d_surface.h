#pragma once

#include "gfx/2d/2d_renderer.h"
#include "gfx/blitter.h"

#ifdef __cplusplus
    #include <cstdbool>
    #include <cstdint>
extern "C" {
#else
    #include <stdbool.h>
    #include <stdint.h>
#endif

typedef struct GFX_2D_Surface {
    GFX_2D_Renderer *renderer;
    uint8_t *buffer;
    GFX_2D_SurfaceDesc desc;
    struct GFX_2D_Surface *back_buffer;
    bool is_locked;
    bool is_dirty;
} GFX_2D_Surface;

GFX_2D_Surface *GFX_2D_Surface_Create(const GFX_2D_SurfaceDesc *desc);
void GFX_2D_Surface_Free(GFX_2D_Surface *surface);

void GFX_2D_Surface_Init(
    GFX_2D_Surface *surface, GFX_2D_Renderer *renderer,
    const GFX_2D_SurfaceDesc *desc);
void GFX_2D_Surface_Close(GFX_2D_Surface *surface);

bool GFX_2D_Surface_Clear(GFX_2D_Surface *surface);
bool GFX_2D_Surface_Blt(
    GFX_2D_Surface *surface, const GFX_BlitterRect *dst_rect,
    GFX_2D_Surface *src, const GFX_BlitterRect *src_rect);
bool GFX_2D_Surface_Flip(GFX_2D_Surface *surface);
GFX_2D_Surface *GFX_2D_Surface_GetAttachedSurface(GFX_2D_Surface *surface);

bool GFX_2D_Surface_Lock(GFX_2D_Surface *surface, GFX_2D_SurfaceDesc *out_desc);
bool GFX_2D_Surface_Unlock(GFX_2D_Surface *surface, LPVOID lp);

#ifdef __cplusplus
}
#endif
