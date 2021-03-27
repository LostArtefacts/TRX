#include "specific/dd.h"

#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/ati.h"
#include "specific/smain.h"
#include "util.h"

#include <stdlib.h>

typedef struct DDLIGHTNING {
    int32_t x1;
    int32_t y1;
    int32_t z1;
    int32_t thickness1;
    int32_t x2;
    int32_t y2;
    int32_t z2;
    int32_t thickness2;
} DDLIGHTNING;

#define DDNormalizeVertices ((int (*)(int num, C3D_VTCF *vertices))0x0040904D)
#define DDLightningTable ARRAY_(0x005DA800, DDLIGHTNING, [100])
#define DDLightningCount VAR_U_(0x00463618, int32_t)

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

    DDDisableTextures();

    ATI3DCIF_RenderPrimList((C3D_VLIST)v_list, 2);

    prim_type = C3D_EPRIM_TRI;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_PRIM_TYPE, &prim_type);
}

void DDDisableTextures()
{
    if (IsTextureMode) {
        int32_t textures_enabled = 0;
        ATI3DCIF_ContextSetState(
            ATIRenderContext, C3D_ERS_TMAP_EN, &textures_enabled);
        IsTextureMode = 0;
    }
}

void DDDrawTranslucentQuad(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    C3D_VTCF vertex[4];
    vertex[0].x = x1;
    vertex[0].y = y1;
    vertex[0].z = 1.0;
    vertex[0].b = 0.0;
    vertex[0].g = 0.0;
    vertex[0].r = 0.0;
    vertex[0].a = 128.0;
    vertex[1].x = x2;
    vertex[1].y = y1;
    vertex[1].z = 1.0;
    vertex[1].b = 0.0;
    vertex[1].g = 0.0;
    vertex[1].r = 0.0;
    vertex[1].a = 128.0;
    vertex[2].x = x2;
    vertex[2].y = y2;
    vertex[2].z = 1.0;
    vertex[2].b = 0.0;
    vertex[2].g = 0.0;
    vertex[2].r = 0.0;
    vertex[2].a = 128.0;
    vertex[3].x = x1;
    vertex[3].y = y2;
    vertex[3].z = 1.0;
    vertex[3].b = 0.0;
    vertex[3].g = 0.0;
    vertex[3].r = 0.0;
    vertex[3].a = 128.0;

    DDDisableTextures();

    int32_t alpha_src = 4;
    int32_t alpha_dst = 5;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);

    DDRenderTriangleStrip(vertex, 4);

    alpha_src = 1;
    alpha_dst = 0;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);
}

void DDDrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2)
{
    DDLightningTable[DDLightningCount].x1 = x1;
    DDLightningTable[DDLightningCount].y1 = y1;
    DDLightningTable[DDLightningCount].z1 = z1;
    DDLightningTable[DDLightningCount].thickness1 = thickness1;
    DDLightningTable[DDLightningCount].x2 = x2;
    DDLightningTable[DDLightningCount].y2 = y2;
    DDLightningTable[DDLightningCount].z2 = z2;
    DDLightningTable[DDLightningCount].thickness2 = thickness2;
    DDLightningCount++;
}

void DDRenderLightningSegment(
    int32_t x1, int32_t y1, int32_t z1, int thickness1, int32_t x2, int32_t y2,
    int32_t z2, int thickness2)
{
    C3D_VTCF vertex[4];

    DDDisableTextures();

    int32_t alpha_src = 4;
    int32_t alpha_dst = 5;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);
    vertex[0].x = x1;
    vertex[0].y = y1;
    vertex[0].z = (double)z1 * 0.0001;
    vertex[0].g = 0.0;
    vertex[0].r = 0.0;
    vertex[0].b = 255.0;
    vertex[0].a = 128.0;

    vertex[1].x = thickness1 / 2 + x1;
    vertex[1].y = vertex[0].y;
    vertex[1].z = vertex[0].z;
    vertex[1].b = 255.0;
    vertex[1].g = 255.0;
    vertex[1].r = 255.0;
    vertex[1].a = 128.0;

    vertex[2].x = (float)(thickness2 / 2 + x2);
    vertex[2].y = (float)y2;
    vertex[2].z = (double)z2 * 0.0001;
    vertex[2].b = 255.0;
    vertex[2].g = 255.0;
    vertex[2].r = 255.0;
    vertex[2].a = 128.0;

    vertex[3].x = (float)x2;
    vertex[3].y = vertex[2].y;
    vertex[3].z = vertex[2].z;
    vertex[3].g = 0.0;
    vertex[3].r = 0.0;
    vertex[3].b = 255.0;
    vertex[3].a = 128.0;

    int num = DDNormalizeVertices(4, vertex);
    if (num) {
        DDRenderTriangleStrip(vertex, num);
    }

    vertex[0].x = thickness1 / 2 + x1;
    vertex[0].y = y1;
    vertex[0].z = (double)z1 * 0.0001;
    vertex[0].b = 255.0;
    vertex[0].g = 255.0;
    vertex[0].r = 255.0;
    vertex[0].a = 128.0;

    vertex[1].x = thickness1 + x1;
    vertex[1].y = vertex[0].y;
    vertex[1].z = vertex[0].z;
    vertex[1].g = 0.0;
    vertex[1].r = 0.0;
    vertex[1].b = 255.0;
    vertex[1].a = 128.0;

    vertex[2].x = (thickness2 + x2);
    vertex[2].y = y2;
    vertex[2].z = z2 * 0.0001;
    vertex[2].g = 0.0;
    vertex[2].r = 0.0;
    vertex[2].b = 255.0;
    vertex[2].a = 128.0;

    vertex[3].x = (thickness2 / 2 + x2);
    vertex[3].y = vertex[2].y;
    vertex[3].z = vertex[2].z;
    vertex[3].b = 255.0;
    vertex[3].g = 255.0;
    vertex[3].r = 255.0;
    vertex[3].a = 128.0;

    num = DDNormalizeVertices(4, vertex);
    if (num) {
        DDRenderTriangleStrip(vertex, num);
    }

    alpha_src = 1;
    alpha_dst = 0;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);
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
    INJECT(0x0040C8E7, DDDrawTranslucentQuad);
    INJECT(0x0040D056, DDDrawLightningSegment);
    INJECT(0x0040CC5D, DDRenderLightningSegment);
}
