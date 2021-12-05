#pragma once

#include "ddraw/ddraw.h"

#ifdef __cplusplus
extern "C" {
#endif

void MyIDirectDraw2_CreateSurface(
    LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface);

void MyIDirectDrawSurface_GetAttachedSurface(
    LPDIRECTDRAWSURFACE p, LPDIRECTDRAWSURFACE *lplpDDAttachedSurface);

HRESULT MyIDirectDrawSurface2_Lock(
    LPDIRECTDRAWSURFACE p, LPDDSURFACEDESC lpDDSurfaceDesc);

HRESULT MyIDirectDrawSurface2_Unlock(LPDIRECTDRAWSURFACE p, LPVOID lp);

HRESULT MyIDirectDrawSurface_Blt(
    LPDIRECTDRAWSURFACE p, LPRECT lpDestRect,
    LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags);

void MyIDirectDrawSurface_Release(LPDIRECTDRAWSURFACE p);

HRESULT MyIDirectDrawSurface_Flip(LPDIRECTDRAWSURFACE p);

#ifdef __cplusplus
}
#endif
