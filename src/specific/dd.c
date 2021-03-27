#include "specific/dd.h"

#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/ati.h"
#include "specific/smain.h"
#include "util.h"

#include <stdlib.h>

void DDError(HRESULT result)
{
    if (result) {
        LOG_ERROR("DirectDraw error code %x", result);
        ShowFatalError("Fatal DirectDraw error!");
    }
}

void DDRenderBegin()
{
    DDOldIsRendering = DDIsRendering;
    if (!DDIsRendering) {
        ATI3DCIF_RenderBegin(ATIRenderContext);
        DDIsRendering = 1;
    }
}

void DDRenderEnd()
{
    DDOldIsRendering = DDIsRendering;
    if (DDIsRendering) {
        ATI3DCIF_RenderEnd();
        DDIsRendering = 0;
    }
}

void DDRenderToggle()
{
    if (DDOldIsRendering) {
        DDRenderBegin();
    } else {
        DDRenderEnd();
    }
}

void DDClearSurface(LPDIRECTDRAWSURFACE surface)
{
    DDBLTFX blt_fx;
    blt_fx.dwSize = sizeof(DDBLTFX);
    blt_fx.dwFillColor = 0;
    HRESULT result = IDirectDrawSurface_Blt(
        surface, NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &blt_fx);
    if (result) {
        DDError(result);
    }
}

void DDBlitSurface(LPDIRECTDRAWSURFACE target, LPDIRECTDRAWSURFACE source)
{
    RECT rect;
    SetRect(&rect, 0, 0, DDrawSurfaceWidth, DDrawSurfaceHeight);
    HRESULT result =
        IDirectDrawSurface_Blt(source, &rect, target, &rect, DDBLT_WAIT, NULL);
    if (result) {
        DDError(result);
    }
}

void DDRenderTriangleStrip(C3D_VTCF *vertices, int num)
{
    ATI3DCIF_RenderPrimStrip(vertices, 3);
    int left = num - 2;
    for (int i = num - 3; i > 0; i--) {
        memcpy(&vertices[1], &vertices[2], left * sizeof(C3D_VTCF));
        ATI3DCIF_RenderPrimStrip(vertices, 3);
        left--;
    }
}

void DDDraw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z, int32_t color)
{
    C3D_VTCF vertex[2];

    vertex[0].x = (float)x1;
    vertex[0].y = (float)y1;
    vertex[0].z = 0.0;
    vertex[0].r = (float)(4 * (char)GamePalette[color].r);
    vertex[0].g = (float)(4 * (char)GamePalette[color].g);
    vertex[0].b = (float)(4 * (char)GamePalette[color].b);

    vertex[1].x = (float)x2;
    vertex[1].y = (float)y2;
    vertex[1].z = 0.0;
    vertex[1].r = vertex[0].r;
    vertex[1].g = vertex[0].g;
    vertex[1].b = vertex[0].b;

    C3D_VTCF *v_list[2] = { &vertex[0], &vertex[1] };

    C3D_EPRIM prim_type = C3D_EPRIM_LINE;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_PRIM_TYPE, &prim_type);

    if (IsTextureMode) {
        int32_t textures_enabled = 0;
        ATI3DCIF_ContextSetState(
            ATIRenderContext, C3D_ERS_TMAP_EN, &textures_enabled);
        IsTextureMode = 0;
    }

    ATI3DCIF_RenderPrimList((C3D_VLIST)v_list, 2);

    prim_type = C3D_EPRIM_TRI;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_PRIM_TYPE, &prim_type);
}

void T1MInjectSpecificDD()
{
    INJECT(0x004077D0, DDError);
    INJECT(0x00407827, DDRenderBegin);
    INJECT(0x0040783B, DDRenderEnd);
    INJECT(0x00407862, DDRenderToggle);
    INJECT(0x00407A49, DDClearSurface);
    INJECT(0x00408B2C, DDBlitSurface);
    INJECT(0x00408E6D, DDRenderTriangleStrip);
    INJECT(0x0040C7EE, DDDraw2DLine);
}
