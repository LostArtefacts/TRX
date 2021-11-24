#include "global/vars_platform.h"

#include "global/const.h"

HINSTANCE g_TombModule;
HWND g_TombHWND;
HMODULE g_GLRage;

LPDIRECTDRAW g_DDraw;
float g_DDrawSurfaceMinX;
float g_DDrawSurfaceMinY;
float g_DDrawSurfaceMaxX;
float g_DDrawSurfaceMaxY;
int32_t g_DDrawSurfaceWidth;
int32_t g_DDrawSurfaceHeight;
LPDIRECTDRAWSURFACE g_Surface1 = NULL;
LPDIRECTDRAWSURFACE g_Surface2 = NULL;
LPDIRECTDRAWSURFACE g_Surface3 = NULL;
LPDIRECTDRAWSURFACE g_Surface4 = NULL;
LPDIRECTDRAWSURFACE g_TextureSurfaces[MAX_TEXTPAGES] = { NULL };
