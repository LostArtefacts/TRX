#include "specific/hwr.h"

#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/ati.h"
#include "specific/display.h"
#include "specific/smain.h"
#include "util.h"

#include <stdlib.h>

typedef struct HWR_LIGHTNING {
    int32_t x1;
    int32_t y1;
    int32_t z1;
    int32_t thickness1;
    int32_t x2;
    int32_t y2;
    int32_t z2;
    int32_t thickness2;
} HWR_LIGHTNING;

#define HWR_LightningTable ARRAY_(0x005DA800, HWR_LIGHTNING, [100])
#define HWR_LightningCount VAR_U_(0x00463618, int32_t)

void HWR_CheckError(HRESULT result)
{
    if (result != DD_OK) {
        LOG_ERROR("DirectDraw error code %x", result);
        ShowFatalError("Fatal DirectDraw error!");
    }
}

void HWR_RenderBegin()
{
    HWR_OldIsRendering = HWR_IsRendering;
    if (!HWR_IsRendering) {
        ATI3DCIF_RenderBegin(ATIRenderContext);
        HWR_IsRendering = 1;
    }
}

void HWR_RenderEnd()
{
    HWR_OldIsRendering = HWR_IsRendering;
    if (HWR_IsRendering) {
        ATI3DCIF_RenderEnd();
        HWR_IsRendering = 0;
    }
}

void HWR_RenderToggle()
{
    if (HWR_OldIsRendering) {
        HWR_RenderBegin();
    } else {
        HWR_RenderEnd();
    }
}

void HWR_ClearSurface(LPDIRECTDRAWSURFACE surface)
{
    DDBLTFX blt_fx;
    blt_fx.dwSize = sizeof(DDBLTFX);
    blt_fx.dwFillColor = 0;
    HRESULT result = IDirectDrawSurface_Blt(
        surface, NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &blt_fx);
    HWR_CheckError(result);
}

void HWR_DumpScreen()
{
    HWR_FlipPrimaryBuffer();
    HWR_SelectedTexture = -1;
}

void HWR_FlipPrimaryBuffer()
{
    HWR_RenderEnd();
    HRESULT result = IDirectDrawSurface_Flip(Surface1, NULL, DDFLIP_WAIT);
    HWR_CheckError(result);
    HWR_RenderToggle();

    void *old_ptr = Surface2DrawPtr;
    Surface2DrawPtr = Surface1DrawPtr;
    Surface1DrawPtr = old_ptr;

    HWR_SetupRenderContextAndRender();
}

void HWR_BlitSurface(LPDIRECTDRAWSURFACE target, LPDIRECTDRAWSURFACE source)
{
    RECT rect;
    SetRect(&rect, 0, 0, DDrawSurfaceWidth, DDrawSurfaceHeight);
    HRESULT result =
        IDirectDrawSurface_Blt(source, &rect, target, &rect, DDBLT_WAIT, NULL);
    HWR_CheckError(result);
}

void HWR_CopyPicture()
{
    LOG_INFO("CopyPictureHardware:");
    if (!Surface3) {
        DDSURFACEDESC surface_desc;
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps =
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
        surface_desc.dwWidth = DDrawSurfaceWidth;
        surface_desc.dwHeight = DDrawSurfaceHeight;
        HRESULT result =
            IDirectDraw2_CreateSurface(DDraw, &surface_desc, &Surface3, 0);
        HWR_CheckError(result);
    }

    HWR_RenderEnd();
    HWR_BlitSurface(Surface2, Surface3);
    HWR_RenderToggle();
    LOG_INFO("    complete");
}

void HWR_DownloadPicture()
{
    LOG_INFO("DownloadPictureHardware:");

    DDSURFACEDESC surface_desc;
    HRESULT result;

    if (!Surface3) {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps =
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
        surface_desc.dwWidth = DDrawSurfaceWidth;
        surface_desc.dwHeight = DDrawSurfaceHeight;
        result = IDirectDraw2_CreateSurface(DDraw, &surface_desc, &Surface3, 0);
        HWR_CheckError(result);
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = 108;

    result =
        IDirectDrawSurface2_Lock(Surface3, NULL, &surface_desc, DDLOCK_WAIT, 0);
    HWR_CheckError(result);

    uint16_t *output_ptr = surface_desc.lpSurface;
    uint8_t *input_ptr = ScrPtr;
    for (int i = 0; i < DDrawSurfaceHeight * DDrawSurfaceWidth; i++) {
        uint8_t idx = *input_ptr++;
        uint16_t r = GamePalette[idx].r & 0x3E;
        uint16_t g = GamePalette[idx].g & 0x3E;
        uint16_t b = GamePalette[idx].b & 0x3E;
        *output_ptr++ = (b >> 1) | (16 * g) | (r << 9);
    }

    result = IDirectDrawSurface2_Unlock(Surface3, surface_desc.lpSurface);
    HWR_CheckError(result);

    LOG_INFO("    complete");
}

void HWR_RenderTriangleStrip(C3D_VTCF *vertices, int num)
{
    ATI3DCIF_RenderPrimStrip(vertices, 3);
    int left = num - 2;
    for (int i = num - 3; i > 0; i--) {
        memcpy(&vertices[1], &vertices[2], left * sizeof(C3D_VTCF));
        ATI3DCIF_RenderPrimStrip(vertices, 3);
        left--;
    }
}

void HWR_Draw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 color1,
    RGB888 color2)
{
    C3D_VTCF vertex[2];

    vertex[0].x = (float)x1;
    vertex[0].y = (float)y1;
    vertex[0].z = 0.0;
    vertex[0].r = color1.r;
    vertex[0].g = color1.g;
    vertex[0].b = color1.b;

    vertex[1].x = (float)x2;
    vertex[1].y = (float)y2;
    vertex[1].z = 0.0;
    vertex[1].r = color2.r;
    vertex[1].g = color2.g;
    vertex[1].b = color2.b;

    C3D_VTCF *v_list[2] = { &vertex[0], &vertex[1] };

    C3D_EPRIM prim_type = C3D_EPRIM_LINE;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_PRIM_TYPE, &prim_type);

    HWR_DisableTextures();

    ATI3DCIF_RenderPrimList((C3D_VLIST)v_list, 2);

    prim_type = C3D_EPRIM_TRI;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_PRIM_TYPE, &prim_type);
}

void HWR_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br)
{
    C3D_VTCF vertex[4];

    vertex[0].x = x1;
    vertex[0].y = y1;
    vertex[0].z = 1.0;
    vertex[0].r = tl.r;
    vertex[0].g = tl.g;
    vertex[0].b = tl.b;

    vertex[1].x = x2;
    vertex[1].y = y1;
    vertex[1].z = 1.0;
    vertex[1].r = tr.r;
    vertex[1].g = tr.g;
    vertex[1].b = tr.b;

    vertex[2].x = x2;
    vertex[2].y = y2;
    vertex[2].z = 1.0;
    vertex[2].r = br.r;
    vertex[2].g = br.g;
    vertex[2].b = br.b;

    vertex[3].x = x1;
    vertex[3].y = y2;
    vertex[3].z = 1.0;
    vertex[3].r = bl.r;
    vertex[3].g = bl.g;
    vertex[3].b = bl.b;

    HWR_DisableTextures();

    HWR_RenderTriangleStrip(vertex, 4);
}

void HWR_DisableTextures()
{
    if (HWR_IsTextureMode) {
        int32_t textures_enabled = 0;
        ATI3DCIF_ContextSetState(
            ATIRenderContext, C3D_ERS_TMAP_EN, &textures_enabled);
        HWR_IsTextureMode = 0;
    }
}

void HWR_DrawTranslucentQuad(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
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

    HWR_DisableTextures();

    int32_t alpha_src = 4;
    int32_t alpha_dst = 5;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);

    HWR_RenderTriangleStrip(vertex, 4);

    alpha_src = 1;
    alpha_dst = 0;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);
}

void HWR_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2)
{
    HWR_LightningTable[HWR_LightningCount].x1 = x1;
    HWR_LightningTable[HWR_LightningCount].y1 = y1;
    HWR_LightningTable[HWR_LightningCount].z1 = z1;
    HWR_LightningTable[HWR_LightningCount].thickness1 = thickness1;
    HWR_LightningTable[HWR_LightningCount].x2 = x2;
    HWR_LightningTable[HWR_LightningCount].y2 = y2;
    HWR_LightningTable[HWR_LightningCount].z2 = z2;
    HWR_LightningTable[HWR_LightningCount].thickness2 = thickness2;
    HWR_LightningCount++;
}

void HWR_RenderLightningSegment(
    int32_t x1, int32_t y1, int32_t z1, int thickness1, int32_t x2, int32_t y2,
    int32_t z2, int thickness2)
{
    C3D_VTCF vertex[4];

    HWR_DisableTextures();

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

    int num = HWR_ClipVertices(4, vertex);
    if (num) {
        HWR_RenderTriangleStrip(vertex, num);
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

    num = HWR_ClipVertices(4, vertex);
    if (num) {
        HWR_RenderTriangleStrip(vertex, num);
    }

    alpha_src = 1;
    alpha_dst = 0;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);
}

int32_t HWR_ClipVertices(int32_t num, C3D_VTCF *source)
{
    float scale;
    C3D_VTCF vertices[10];

    C3D_VTCF *l = &source[num - 1];
    int j = 0;

    for (int i = 0; i < num; i++) {
        C3D_VTCF *v1 = &vertices[j];
        C3D_VTCF *v2 = l;
        l = &source[i];

        if (v2->x < DDrawSurfaceMinX) {
            if (l->x < DDrawSurfaceMinX) {
                continue;
            }
            scale = (DDrawSurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = DDrawSurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        } else if (v2->x > DDrawSurfaceMaxX) {
            if (l->x > DDrawSurfaceMaxX) {
                continue;
            }
            scale = (DDrawSurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = DDrawSurfaceMaxX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        }

        if (l->x < DDrawSurfaceMinX) {
            scale = (DDrawSurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = DDrawSurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        } else if (l->x > DDrawSurfaceMaxX) {
            scale = (DDrawSurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = DDrawSurfaceMaxX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        } else {
            v1->x = l->x;
            v1->y = l->y;
            v1->z = l->z;
            v1->r = l->r;
            v1->g = l->g;
            v1->b = l->b;
            v1->a = l->a;
            v1 = &vertices[++j];
        }
    }

    if (j < 3) {
        return 0;
    }

    num = j;
    l = &vertices[j - 1];
    j = 0;

    for (int i = 0; i < num; i++) {
        C3D_VTCF *v1 = &source[j];
        C3D_VTCF *v2 = l;
        l = &vertices[i];

        if (v2->y < DDrawSurfaceMinY) {
            if (l->y < DDrawSurfaceMinY) {
                continue;
            }
            scale = (DDrawSurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = DDrawSurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        } else if (v2->y > DDrawSurfaceMaxY) {
            if (l->y > DDrawSurfaceMaxY) {
                continue;
            }
            scale = (DDrawSurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = DDrawSurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        }

        if (l->y < DDrawSurfaceMinY) {
            scale = (DDrawSurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = DDrawSurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        } else if (l->y > DDrawSurfaceMaxY) {
            scale = (DDrawSurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = DDrawSurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        } else {
            v1->x = l->x;
            v1->y = l->y;
            v1->z = l->z;
            v1->r = l->r;
            v1->g = l->g;
            v1->b = l->b;
            v1->a = l->a;
            v1 = &source[++j];
        }
    }

    if (j < 3) {
        return 0;
    }

    return j;
}

int32_t HWR_ClipVertices2(int32_t num, C3D_VTCF *source)
{
    float scale;
    C3D_VTCF vertices[8];

    C3D_VTCF *l = &source[num - 1];
    int j = 0;

    for (int i = 0; i < num; i++) {
        C3D_VTCF *v1 = &vertices[j];
        C3D_VTCF *v2 = l;
        l = &source[i];

        if (v2->x < DDrawSurfaceMinX) {
            if (l->x < DDrawSurfaceMinX) {
                continue;
            }
            scale = (DDrawSurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = DDrawSurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        } else if (v2->x > DDrawSurfaceMaxX) {
            if (l->x > DDrawSurfaceMaxX) {
                continue;
            }
            scale = (DDrawSurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = DDrawSurfaceMaxX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        }

        if (l->x < DDrawSurfaceMinX) {
            scale = (DDrawSurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = DDrawSurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        } else if (l->x > DDrawSurfaceMaxX) {
            scale = (DDrawSurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = DDrawSurfaceMaxX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        } else {
            v1->x = l->x;
            v1->y = l->y;
            v1->z = l->z;
            v1->r = l->r;
            v1->g = l->g;
            v1->b = l->b;
            v1->w = l->w;
            v1->s = l->s;
            v1->t = l->t;
            v1 = &vertices[++j];
        }
    }

    if (j < 3) {
        return 0;
    }

    num = j;
    l = &vertices[j - 1];
    j = 0;

    for (int i = 0; i < num; i++) {
        C3D_VTCF *v1 = &source[j];
        C3D_VTCF *v2 = l;
        l = &vertices[i];

        if (v2->y < DDrawSurfaceMinY) {
            if (l->y < DDrawSurfaceMinY) {
                continue;
            }
            scale = (DDrawSurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = DDrawSurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        } else if (v2->y > DDrawSurfaceMaxY) {
            if (l->y > DDrawSurfaceMaxY) {
                continue;
            }
            scale = (DDrawSurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = DDrawSurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        }

        if (l->y < DDrawSurfaceMinY) {
            scale = (DDrawSurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = DDrawSurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        } else if (l->y > DDrawSurfaceMaxY) {
            scale = (DDrawSurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = DDrawSurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        } else {
            v1->x = l->x;
            v1->y = l->y;
            v1->z = l->z;
            v1->r = l->r;
            v1->g = l->g;
            v1->b = l->b;
            v1->w = l->w;
            v1->s = l->s;
            v1->t = l->t;
            v1 = &source[++j];
        }
    }

    if (j < 3) {
        return 0;
    }

    return j;
}

void HWR_FadeToPal(int32_t fade_value, RGB888 *palette)
{
    // null sub
}

void HWR_FadeWait()
{
    HWR_ClearSurfaceDepth();
    HWR_DumpScreen();
}

void HWR_SwitchResolution()
{
    if (HiRes == 0) {
        GameVidWidth = 320;
        GameVidHeight = 200;
    } else if (HiRes == 1) {
        GameVidWidth = 512;
        GameVidHeight = 384;
    } else if (HiRes == 3) {
        GameVidWidth = GetSystemMetrics(SM_CXSCREEN);
        GameVidHeight = GetSystemMetrics(SM_CYSCREEN);
    } else {
        GameVidWidth = 640;
        GameVidHeight = 480;
    }

    HWR_SetHardwareVideoMode();
    SetupScreenSize();
}

void T1MInjectSpecificHWR()
{
    INJECT(0x004077D0, HWR_CheckError);
    INJECT(0x00407827, HWR_RenderBegin);
    INJECT(0x0040783B, HWR_RenderEnd);
    INJECT(0x00407862, HWR_RenderToggle);
    INJECT(0x004079E9, HWR_FlipPrimaryBuffer);
    INJECT(0x00407A49, HWR_ClearSurface);
    INJECT(0x004089F4, HWR_SwitchResolution);
    INJECT(0x00408A70, HWR_DumpScreen);
    INJECT(0x00408B2C, HWR_BlitSurface);
    INJECT(0x00408B85, HWR_CopyPicture);
    INJECT(0x00408C3A, HWR_DownloadPicture);
    INJECT(0x00408E32, HWR_FadeWait);
    INJECT(0x00408E6D, HWR_RenderTriangleStrip);
    INJECT(0x0040904D, HWR_ClipVertices);
    INJECT(0x0040A6B1, HWR_ClipVertices2);
    INJECT(0x0040C7EE, HWR_Draw2DLine);
    INJECT(0x0040C8E7, HWR_DrawTranslucentQuad);
    INJECT(0x0040CC5D, HWR_RenderLightningSegment);
    INJECT(0x0040D056, HWR_DrawLightningSegment);
}
