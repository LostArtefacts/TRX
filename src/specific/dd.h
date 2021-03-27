#ifndef T1M_SPECIFIC_DD_H
#define T1M_SPECIFIC_DD_H

#include "specific/ati.h"

#include <windows.h>
#include <ddraw.h>
#include <stdint.h>

void DDError(HRESULT result);
void DDRenderBegin();
void DDRenderEnd();
void DDRenderToggle();
void DDClearSurface(LPDIRECTDRAWSURFACE surface);
void DDBlitSurface(LPDIRECTDRAWSURFACE target, LPDIRECTDRAWSURFACE source);
void DDRenderTriangleStrip(C3D_VTCF *vertices, int num);
void DDDraw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z, int32_t color);

void T1MInjectSpecificDD();

#endif
