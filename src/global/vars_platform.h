#ifndef T1M_GLOBAL_VARS_PLATFORM_H
#define T1M_GLOBAL_VARS_PLATFORM_H

#include "global/const.h"
#include "specific/s_ati.h"

#include <ddraw.h>
#include <dsound.h>
#include <windows.h>

extern HINSTANCE TombModule;
extern HWND TombHWND;

extern LPDIRECTDRAW DDraw;
extern float DDrawSurfaceMinX;
extern float DDrawSurfaceMinY;
extern float DDrawSurfaceMaxX;
extern float DDrawSurfaceMaxY;
extern int32_t DDrawSurfaceWidth;
extern int32_t DDrawSurfaceHeight;
extern LPDIRECTDRAWSURFACE Surface1;
extern LPDIRECTDRAWSURFACE Surface2;
extern LPDIRECTDRAWSURFACE Surface3;
extern LPDIRECTDRAWSURFACE Surface4;
extern LPDIRECTDRAWSURFACE TextureSurfaces[MAX_TEXTPAGES];

extern C3D_HTX ATITextureMap[MAX_TEXTPAGES];
extern C3D_HTXPAL ATITexturePalette;
extern C3D_PALETTENTRY ATIPalette[256];
extern C3D_COLOR ATIChromaKey;

#endif
