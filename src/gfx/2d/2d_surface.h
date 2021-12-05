#pragma once

#include "ddraw/ddraw.h"
#include "gfx/2d/2d_renderer.h"

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
    DDSURFACEDESC desc;
    struct GFX_2D_Surface *back_buffer;
    bool is_locked;
    bool is_dirty;
} GFX_2D_Surface;

GFX_2D_Surface *GFX_2D_Surface_Create(LPDDSURFACEDESC lpDDSurfaceDesc);
void GFX_2D_Surface_Free(GFX_2D_Surface *surface);

void GFX_2D_Surface_Init(
    GFX_2D_Surface *surface, GFX_2D_Renderer *renderer,
    LPDDSURFACEDESC lpDDSurfaceDesc);
void GFX_2D_Surface_Close(GFX_2D_Surface *surface);

bool GFX_2D_Surface_Clear(GFX_2D_Surface *surface);
bool GFX_2D_Surface_Blt(
    GFX_2D_Surface *surface, LPRECT lpDestRect, GFX_2D_Surface *lpDDSrcSurface,
    LPRECT lpSrcRect);
bool GFX_2D_Surface_Flip(GFX_2D_Surface *surface);
GFX_2D_Surface *GFX_2D_Surface_GetAttachedSurface(GFX_2D_Surface *surface);

bool GFX_2D_Surface_Lock(
    GFX_2D_Surface *surface, LPDDSURFACEDESC lpDDSurfaceDesc);
bool GFX_2D_Surface_Unlock(GFX_2D_Surface *surface, LPVOID lp);

#ifdef __cplusplus
}
#endif
