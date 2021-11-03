#include "global/vars_platform.h"

#include "global/const.h"

HINSTANCE TombModule;

LPDIRECTSOUND DSound;
uint32_t AuxDeviceID;
uint32_t MCIDeviceID;

LPDIRECTDRAW DDraw;
float DDrawSurfaceMinX;
float DDrawSurfaceMinY;
float DDrawSurfaceMaxX;
float DDrawSurfaceMaxY;
int32_t DDrawSurfaceWidth;
int32_t DDrawSurfaceHeight;
LPDIRECTDRAWSURFACE Surface1 = NULL;
LPDIRECTDRAWSURFACE Surface2 = NULL;
LPDIRECTDRAWSURFACE Surface3 = NULL;
LPDIRECTDRAWSURFACE Surface4 = NULL;
LPDIRECTDRAWSURFACE TextureSurfaces[MAX_TEXTPAGES] = { NULL };
void *Surface1DrawPtr = NULL;
void *Surface2DrawPtr = NULL;

HMODULE HATI3DCIFModule;
C3D_HRC ATIRenderContext;
C3D_3DCIFINFO ATIInfo;
C3D_HTX ATITextureMap[MAX_TEXTPAGES];
C3D_HTXPAL ATITexturePalette;
C3D_PALETTENTRY ATIPalette[256];
C3D_COLOR ATIChromaKey;
