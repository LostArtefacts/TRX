#pragma once

#include <windows.h>

#define DDSCAPS_FLIP 0x00000010
#define DDSCAPS_FRONTBUFFER 0x00000020
#define DDSCAPS_PRIMARYSURFACE 0x00000200

typedef struct _DDSURFACEDESC {
    DWORD dwHeight;
    DWORD dwWidth;
    LONG lPitch;
    DWORD dwBackBufferCount;
    LPVOID lpSurface;
    DWORD dwRGBBitCount;
    DWORD dwCaps;
} DDSURFACEDESC, *LPDDSURFACEDESC;
