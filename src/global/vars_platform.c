#include "global/vars_platform.h"

#include "global/const.h"

HINSTANCE TombModule;

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
