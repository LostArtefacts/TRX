#pragma once

#include "ddraw/2d_surface.h"
#include "ddraw/ddraw.h"

#ifdef __cplusplus
extern "C" {
#endif

GFX_2D_Surface *MyIDirectDraw2_CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc);

GFX_2D_Surface *MyIDirectDrawSurface_GetAttachedSurface(GFX_2D_Surface *p);

HRESULT MyIDirectDrawSurface2_Lock(
    GFX_2D_Surface *p, LPDDSURFACEDESC lpDDSurfaceDesc);

HRESULT MyIDirectDrawSurface2_Unlock(GFX_2D_Surface *p, LPVOID lp);

HRESULT MyIDirectDrawSurface_Blt(
    GFX_2D_Surface *p, LPRECT lpDestRect, GFX_2D_Surface *lpDDSrcSurface,
    LPRECT lpSrcRect, DWORD dwFlags);

void MyIDirectDrawSurface_Release(GFX_2D_Surface *p);

HRESULT MyIDirectDrawSurface_Flip(GFX_2D_Surface *p);

#ifdef __cplusplus
}
#endif
