#ifndef T1M_GLOBAL_VARS_PLATFORM_H
#define T1M_GLOBAL_VARS_PLATFORM_H

#include "specific/ati.h"

#include <ddraw.h>
#include <dsound.h>
#include <windows.h>

extern HINSTANCE TombModule;
extern HWND TombHWND;

extern LPDIRECTSOUND DSound;
extern uint32_t AuxDeviceID;
extern uint32_t MCIDeviceID;

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
extern LPDIRECTDRAWSURFACE TextureSurfaces[];
extern void *Surface1DrawPtr;
extern void *Surface2DrawPtr;

extern HMODULE HATI3DCIFModule;
extern C3D_HRC ATIRenderContext;
extern C3D_3DCIFINFO ATIInfo;
extern C3D_HTX ATITextureMap[];
extern C3D_HTXPAL ATITexturePalette;
extern C3D_PALETTENTRY ATIPalette[];
extern C3D_COLOR ATIChromaKey;

#endif
