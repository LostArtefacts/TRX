#ifndef T1M_SPECIFIC_DD_H
#define T1M_SPECIFIC_DD_H

#include "specific/ati.h"

#include <windows.h>
#include <ddraw.h>
#include <stdint.h>

// TODO: function naming, Render vs. Draw vs. Insert
// TODO: port ATI3DCIF to actual D3D calls

void DDError(HRESULT result);
void DDRenderBegin();
void DDRenderEnd();
void DDRenderToggle();
void DDDisableTextures();
void DDClearSurface(LPDIRECTDRAWSURFACE surface);
void DDBlitSurface(LPDIRECTDRAWSURFACE target, LPDIRECTDRAWSURFACE source);
void DDRenderTriangleStrip(C3D_VTCF *vertices, int num);
void DDDraw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z, int32_t color);
void DDDrawTranslucentQuad(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void DDDrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2);

void T1MInjectSpecificDD();

#endif
