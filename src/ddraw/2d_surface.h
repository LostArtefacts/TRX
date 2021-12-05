#pragma once

#include "ddraw/ddraw.h"
#include "gfx/2d/2d_renderer.h"

#ifdef __cplusplus
    #include <cstdint>
extern "C" {
#else
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

void GFX_2D_Surface_Init(
    GFX_2D_Surface *surface, GFX_2D_Renderer *renderer,
    LPDDSURFACEDESC lpDDSurfaceDesc);
void GFX_2D_Surface_Close(GFX_2D_Surface *surface);

HRESULT GFX_2D_Surface_Blt(
    GFX_2D_Surface *surface, LPRECT lpDestRect, GFX_2D_Surface *lpDDSrcSurface,
    LPRECT lpSrcRect, DWORD dwFlags);
HRESULT GFX_2D_Surface_Flip(GFX_2D_Surface *surface);
GFX_2D_Surface *GFX_2D_Surface_GetAttachedSurface(GFX_2D_Surface *surface);
HRESULT GFX_2D_Surface_GetPixelFormat(
    GFX_2D_Surface *surface, LPDDPIXELFORMAT lpDDPixelFormat);
HRESULT GFX_2D_Surface_GetSurfaceDesc(
    GFX_2D_Surface *surface, LPDDSURFACEDESC lpDDSurfaceDesc);

HRESULT GFX_2D_Surface_Lock(
    GFX_2D_Surface *surface, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT GFX_2D_Surface_Unlock(GFX_2D_Surface *surface, LPVOID lp);

#ifdef __cplusplus
}
#endif
