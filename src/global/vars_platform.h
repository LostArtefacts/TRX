#ifndef T1M_GLOBAL_VARS_PLATFORM_H
#define T1M_GLOBAL_VARS_PLATFORM_H

#include "global/const.h"
#include "specific/s_ati.h"

#include <ddraw.h>
#include <dsound.h>
#include <windows.h>

extern HINSTANCE g_TombModule;
extern HWND g_TombHWND;
extern HMODULE g_GLRage;

extern LPDIRECTDRAW g_DDraw;
extern float g_DDrawSurfaceMinX;
extern float g_DDrawSurfaceMinY;
extern float g_DDrawSurfaceMaxX;
extern float g_DDrawSurfaceMaxY;
extern int32_t g_DDrawSurfaceWidth;
extern int32_t g_DDrawSurfaceHeight;
extern LPDIRECTDRAWSURFACE g_Surface1;
extern LPDIRECTDRAWSURFACE g_Surface2;
extern LPDIRECTDRAWSURFACE g_Surface3;
extern LPDIRECTDRAWSURFACE g_Surface4;
extern LPDIRECTDRAWSURFACE g_TextureSurfaces[MAX_TEXTPAGES];

extern C3D_HTX g_ATITextureMap[MAX_TEXTPAGES];
extern C3D_HTXPAL g_ATITexturePalette;
extern C3D_PALETTENTRY g_ATIPalette[256];
extern C3D_COLOR g_ATIChromaKey;

#endif
