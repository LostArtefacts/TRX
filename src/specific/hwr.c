#include "specific/hwr.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/ati.h"
#include "specific/display.h"
#include "specific/smain.h"
#include "util.h"

#include <stdlib.h>
#include <assert.h>

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

static void HWR_EnableTextureMode(void);
static void HWR_DisableTextureMode(void);

static void HWR_EnableTextureMode(void)
{
    BOOL enable;

    if (HWR_IsTextureMode) {
        return;
    }

    HWR_IsTextureMode = 1;
    enable = TRUE;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_TMAP_EN, &enable);
}

static void HWR_DisableTextureMode(void)
{
    BOOL enable;

    if (!HWR_IsTextureMode) {
        return;
    }

    HWR_IsTextureMode = 0;
    enable = FALSE;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_TMAP_EN, &enable);
}

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

void HWR_GetSurfaceAndPitch(
    LPDIRECTDRAWSURFACE surface, LPVOID *out_surface, int32_t *out_pitch)
{
    DDSURFACEDESC surface_desc;
    HRESULT result;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    result =
        IDirectDrawSurface2_Lock(surface, NULL, &surface_desc, DDLOCK_WAIT, 0);
    HWR_CheckError(result);

    if (out_surface) {
        *out_surface = surface_desc.lpSurface;
    }
    if (out_pitch) {
        *out_pitch = surface_desc.lPitch / 2;
    }

    result = IDirectDrawSurface2_Unlock(surface, surface_desc.lpSurface);
    HWR_CheckError(result);
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

void HWR_ReleaseSurfaces()
{
    int i;
    HRESULT result;

    if (Surface1) {
        HWR_ClearSurface(Surface1);
        HWR_ClearSurface(Surface2);

        result = IDirectDrawSurface_Release(Surface1);
        HWR_CheckError(result);
        Surface1 = NULL;
        Surface2 = NULL;
    }

    if (Surface4) {
        result = IDirectDrawSurface_Release(Surface4);
        HWR_CheckError(result);
        Surface4 = NULL;
    }

    for (i = 0; i < MAX_TEXTPAGES; i++) {
        if (TextureSurfaces[i]) {
            result = IDirectDrawSurface_Release(TextureSurfaces[i]);
            HWR_CheckError(result);
            TextureSurfaces[i] = NULL;
        }
    }

    if (Surface3) {
        result = IDirectDrawSurface_Release(Surface3);
        HWR_CheckError(result);
        Surface3 = NULL;
    }
}

void HWR_SetPalette()
{
    int32_t i;

    LOG_INFO("PaletteSetHardware:");

    ATIPalette[0].r = 0;
    ATIPalette[0].g = 0;
    ATIPalette[0].b = 0;
    ATIPalette[0].flags = C3D_LOAD_PALETTE_ENTRY;

    for (i = 1; i < 256; i++) {
        if (GamePalette[i].r || GamePalette[i].g || GamePalette[i].b) {
            ATIPalette[i].r = 4 * GamePalette[i].r;
            ATIPalette[i].g = 4 * GamePalette[i].g;
            ATIPalette[i].b = 4 * GamePalette[i].b;
        } else {
            ATIPalette[i].r = 1;
            ATIPalette[i].g = 1;
            ATIPalette[i].b = 1;
        }
        ATIPalette[i].flags = C3D_LOAD_PALETTE_ENTRY;
    }

    ATIChromaKey.r = 0;
    ATIChromaKey.g = 0;
    ATIChromaKey.b = 0;
    ATIChromaKey.a = 0;

    HWR_IsPaletteActive = 1;
    LOG_INFO("    complete");
}

void HWR_DumpScreen()
{
    HWR_FlipPrimaryBuffer();
    HWR_SelectedTexture = -1;
}

void HWR_ClearSurfaceDepth()
{
    DDBLTFX bltfx;
    HRESULT result;

    HWR_RenderEnd();
    HWR_ClearSurface(Surface2);

    bltfx.dwSize = sizeof(bltfx);
    bltfx.dwFillDepth = 0xFFFF;
    result = IDirectDrawSurface_Blt(
        Surface4, NULL, NULL, NULL, DDBLT_WAIT | DDBLT_DEPTHFILL, &bltfx);
    HWR_CheckError(result);

    HWR_RenderToggle();
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
            IDirectDraw2_CreateSurface(DDraw, &surface_desc, &Surface3, NULL);
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
        result =
            IDirectDraw2_CreateSurface(DDraw, &surface_desc, &Surface3, NULL);
        HWR_CheckError(result);
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);

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

void HWR_SelectTexture(int tex_num)
{
    if (tex_num == HWR_SelectedTexture) {
        return;
    }

    if (!ATITextureMap[tex_num]) {
        ShowFatalError("ERROR: Attempt to select unloaded texture");
    }

    if (ATI3DCIF_ContextSetState(
            ATIRenderContext, C3D_ERS_TMAP_SELECT, &ATITextureMap[tex_num])) {
        LOG_ERROR("    Texture error");
    }

    HWR_SelectedTexture = tex_num;
}

void HWR_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int y2, int z, int sprnum, int shade)
{
    C3D_FLOAT32 t1;
    C3D_FLOAT32 t2;
    C3D_FLOAT32 t3;
    C3D_FLOAT32 t4;
    C3D_FLOAT32 t5;
    C3D_FLOAT32 vz;
    C3D_FLOAT32 vshade;
    int32_t vertex_count;
    PHD_SPRITE *sprite;
    C3D_VTCF vertices[10];
    float multiplier;

    multiplier = 0.0625f * T1MConfig.brightness;

    sprite = &PhdSpriteInfo[sprnum];
    vshade = (8192.0f - shade) * multiplier;
    if (vshade >= 256.0f) {
        vshade = 255.0f;
    }

    t1 = ((int)sprite->offset & 0xFF) + 0.5f;
    t2 = ((int)sprite->offset >> 8) + 0.5f;
    t3 = ((int)sprite->width >> 8) + t1;
    t4 = ((int)sprite->height >> 8) + t2;
    vz = z * 0.0001f;
    t5 = 65536.0f / z;

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = vz;
    vertices[0].s = t1 * t5 * 0.00390625f;
    vertices[0].t = t2 * t5 * 0.00390625f;
    vertices[0].w = t5;
    vertices[0].r = vshade;
    vertices[0].g = vshade;
    vertices[0].b = vshade;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = vz;
    vertices[1].s = t3 * t5 * 0.00390625f;
    vertices[1].t = t2 * t5 * 0.00390625f;
    vertices[1].w = t5;
    vertices[1].r = vshade;
    vertices[1].g = vshade;
    vertices[1].b = vshade;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = vz;
    vertices[2].s = t3 * t5 * 0.00390625f;
    vertices[2].t = t4 * t5 * 0.00390625f;
    vertices[2].w = t5;
    vertices[2].r = vshade;
    vertices[2].g = vshade;
    vertices[2].b = vshade;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = vz;
    vertices[3].s = t1 * t5 * 0.00390625f;
    vertices[3].t = t4 * t5 * 0.00390625f;
    vertices[3].w = t5;
    vertices[3].r = vshade;
    vertices[3].g = vshade;
    vertices[3].b = vshade;

    vertex_count = 4;
    if (x1 < 0 || y1 < 0 || x2 > PhdWinWidth || y2 > PhdWinHeight) {
        vertex_count = HWR_ClipVertices2(vertex_count, vertices);
    }

    if (!vertex_count) {
        return;
    }

    if (HWR_TextureLoaded[sprite->tpage]) {
        HWR_EnableTextureMode();
        HWR_SelectTexture(sprite->tpage);
    }

    // NOTE: original .exe has some additional logic for the case when
    // the requested texture page was not loaded.

    HWR_RenderTriangleStrip(vertices, vertex_count);
}

void HWR_Draw2DLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGB888 color1,
    RGB888 color2)
{
    C3D_VTCF vertices[2];

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = 0.0f;
    vertices[0].r = color1.r;
    vertices[0].g = color1.g;
    vertices[0].b = color1.b;

    vertices[1].x = x2;
    vertices[1].y = y2;
    vertices[1].z = 0.0f;
    vertices[1].r = color2.r;
    vertices[1].g = color2.g;
    vertices[1].b = color2.b;

    C3D_VTCF *v_list[2] = { &vertices[0], &vertices[1] };

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
    C3D_VTCF vertices[4];

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = 1.0f;
    vertices[0].r = tl.r;
    vertices[0].g = tl.g;
    vertices[0].b = tl.b;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = 1.0f;
    vertices[1].r = tr.r;
    vertices[1].g = tr.g;
    vertices[1].b = tr.b;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = 1.0f;
    vertices[2].r = br.r;
    vertices[2].g = br.g;
    vertices[2].b = br.b;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = 1.0f;
    vertices[3].r = bl.r;
    vertices[3].g = bl.g;
    vertices[3].b = bl.b;

    HWR_DisableTextures();

    HWR_RenderTriangleStrip(vertices, 4);
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
    C3D_VTCF vertices[4];

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = 1.0f;
    vertices[0].b = 0.0f;
    vertices[0].g = 0.0f;
    vertices[0].r = 0.0f;
    vertices[0].a = 128.0f;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = 1.0f;
    vertices[1].b = 0.0f;
    vertices[1].g = 0.0f;
    vertices[1].r = 0.0f;
    vertices[1].a = 128.0f;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = 1.0f;
    vertices[2].b = 0.0f;
    vertices[2].g = 0.0f;
    vertices[2].r = 0.0f;
    vertices[2].a = 128.0f;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = 1.0f;
    vertices[3].b = 0.0f;
    vertices[3].g = 0.0f;
    vertices[3].r = 0.0f;
    vertices[3].a = 128.0f;

    HWR_DisableTextures();

    int32_t alpha_src = 4;
    int32_t alpha_dst = 5;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);

    HWR_RenderTriangleStrip(vertices, 4);

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

void HWR_PrintShadow(PHD_VBUF *vbufs, int clip, int vertex_count)
{
    // needs to be more than 8 cause clipping might return more polygons.
    C3D_VTCF vertices[vertex_count * HWR_CLIP_VERTCOUNT_SCALE];
    int i;
    int32_t tmp;

    for (i = 0; i < vertex_count; i++) {
        C3D_VTCF *vertex = &vertices[i];
        PHD_VBUF *vbuf = &vbufs[i];
        vertex->x = vbuf->xs;
        vertex->y = vbuf->ys;
        vertex->z = vbuf->zv * 0.0001f - 16.0f;
        vertex->b = 0.0f;
        vertex->g = 0.0f;
        vertex->r = 0.0f;
        vertex->a = 128.0f;
    }

    if (clip) {
        int original = vertex_count;
        vertex_count = HWR_ClipVertices(vertex_count, vertices);
        assert(vertex_count < original * HWR_CLIP_VERTCOUNT_SCALE);
    }

    if (!vertex_count) {
        return;
    }

    HWR_DisableTextureMode();

    tmp = C3D_EASRC_SRCALPHA;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &tmp);
    tmp = C3D_EADST_INVSRCALPHA;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &tmp);
    HWR_RenderTriangleStrip(vertices, vertex_count);
    tmp = C3D_EASRC_ONE;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &tmp);
    tmp = C3D_EADST_ZERO;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &tmp);
}

void HWR_RenderLightningSegment(
    int32_t x1, int32_t y1, int32_t z1, int thickness1, int32_t x2, int32_t y2,
    int32_t z2, int thickness2)
{
    C3D_VTCF vertices[4];

    HWR_DisableTextures();

    int32_t alpha_src = 4;
    int32_t alpha_dst = 5;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);
    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = z1 * 0.0001f;
    vertices[0].g = 0.0f;
    vertices[0].r = 0.0f;
    vertices[0].b = 255.0f;
    vertices[0].a = 128.0f;

    vertices[1].x = thickness1 / 2 + x1;
    vertices[1].y = vertices[0].y;
    vertices[1].z = vertices[0].z;
    vertices[1].b = 255.0f;
    vertices[1].g = 255.0f;
    vertices[1].r = 255.0f;
    vertices[1].a = 128.0f;

    vertices[2].x = thickness2 / 2 + x2;
    vertices[2].y = y2;
    vertices[2].z = z2 * 0.0001f;
    vertices[2].b = 255.0f;
    vertices[2].g = 255.0f;
    vertices[2].r = 255.0f;
    vertices[2].a = 128.0f;

    vertices[3].x = x2;
    vertices[3].y = vertices[2].y;
    vertices[3].z = vertices[2].z;
    vertices[3].g = 0.0f;
    vertices[3].r = 0.0f;
    vertices[3].b = 255.0f;
    vertices[3].a = 128.0f;

    int num = HWR_ClipVertices(4, vertices);
    if (num) {
        HWR_RenderTriangleStrip(vertices, num);
    }

    vertices[0].x = thickness1 / 2 + x1;
    vertices[0].y = y1;
    vertices[0].z = z1 * 0.0001f;
    vertices[0].b = 255.0f;
    vertices[0].g = 255.0f;
    vertices[0].r = 255.0f;
    vertices[0].a = 128.0f;

    vertices[1].x = thickness1 + x1;
    vertices[1].y = vertices[0].y;
    vertices[1].z = vertices[0].z;
    vertices[1].g = 0.0f;
    vertices[1].r = 0.0f;
    vertices[1].b = 255.0f;
    vertices[1].a = 128.0f;

    vertices[2].x = (thickness2 + x2);
    vertices[2].y = y2;
    vertices[2].z = z2 * 0.0001f;
    vertices[2].g = 0.0f;
    vertices[2].r = 0.0f;
    vertices[2].b = 255.0f;
    vertices[2].a = 128.0f;

    vertices[3].x = (thickness2 / 2 + x2);
    vertices[3].y = vertices[2].y;
    vertices[3].z = vertices[2].z;
    vertices[3].b = 255.0f;
    vertices[3].g = 255.0f;
    vertices[3].r = 255.0f;
    vertices[3].a = 128.0f;

    num = HWR_ClipVertices(4, vertices);
    if (num) {
        HWR_RenderTriangleStrip(vertices, num);
    }

    alpha_src = 1;
    alpha_dst = 0;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_ALPHA_DST, &alpha_dst);
}

int32_t HWR_ClipVertices(int32_t num, C3D_VTCF *source)
{
    float scale;
    C3D_VTCF vertices[num * HWR_CLIP_VERTCOUNT_SCALE];

    C3D_VTCF *l = &source[num - 1];
    int j = 0;
    int i;

    for (i = 0; i < num; i++) {
        assert(j < num * HWR_CLIP_VERTCOUNT_SCALE);
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

    for (i = 0; i < num; i++) {
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
    GameVidWidth = AvailableResolutions[HiRes].width;
    GameVidHeight = AvailableResolutions[HiRes].height;

    HWR_SetHardwareVideoMode();
    SetupScreenSize();
}

int32_t HWR_SetHardwareVideoMode()
{
    DDSURFACEDESC surface_desc;
    HRESULT result;

    LOG_INFO("SetHardwareVideoMode:");
    HWR_ReleaseSurfaces();

    DDrawSurfaceWidth = AvailableResolutions[HiRes].width;
    DDrawSurfaceHeight = AvailableResolutions[HiRes].height;
    DDrawSurfaceMaxX = AvailableResolutions[HiRes].width - 1.0f;
    DDrawSurfaceMaxY = AvailableResolutions[HiRes].height - 1.0f;

    LOG_INFO("    Switching to %dx%d", DDrawSurfaceWidth, DDrawSurfaceHeight);
    result = IDirectDraw_SetDisplayMode(
        DDraw, DDrawSurfaceWidth, DDrawSurfaceHeight, 16);
    HWR_CheckError(result);

    LOG_INFO("    Allocating front/back buffers");
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_PRIMARYSURFACE
        | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
    surface_desc.dwBackBufferCount = 1;
    result = IDirectDraw2_CreateSurface(DDraw, &surface_desc, &Surface1, NULL);
    HWR_CheckError(result);

    HWR_ClearSurface(Surface1);
    LOG_INFO("    Picking up back buffer");
    DDSCAPS caps = { DDSCAPS_BACKBUFFER };
    result = IDirectDrawSurface_GetAttachedSurface(Surface1, &caps, &Surface2);
    HWR_CheckError(result);

    HWR_ClearSurface(Surface2);
    LOG_INFO("    Allocating Z-buffer");
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags =
        DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_ZBUFFERBITDEPTH;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
    surface_desc.dwWidth = DDrawSurfaceWidth;
    surface_desc.dwHeight = DDrawSurfaceHeight;
    surface_desc.dwZBufferBitDepth = 16;
    result = IDirectDraw2_CreateSurface(DDraw, &surface_desc, &Surface4, NULL);
    HWR_CheckError(result);

    LOG_INFO("    Creating texture surfaces");
    for (int i = 0; i < 16; i++) {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags =
            DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
        surface_desc.ddpfPixelFormat.dwSize =
            sizeof(surface_desc.ddpfPixelFormat);
        surface_desc.ddpfPixelFormat.dwRGBBitCount = 8;
        surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
        surface_desc.dwWidth = 256;
        surface_desc.dwHeight = 256;
        result = IDirectDraw2_CreateSurface(
            DDraw, &surface_desc, &TextureSurfaces[i], NULL);
        HWR_CheckError(result);
    }

    void *surface;
    int32_t pitch;
    HWR_GetSurfaceAndPitch(Surface2, &Surface2DrawPtr, &pitch);
    HWR_GetSurfaceAndPitch(Surface1, &Surface1DrawPtr, &pitch);
    LOG_INFO(
        "Pitch = %x Draw1Ptr = %x Draw2Ptr = %x", pitch, Surface1DrawPtr,
        Surface2DrawPtr);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_SURF_DRAW_PITCH, &pitch);
    HWR_SetupRenderContextAndRender();

    HWR_RenderEnd();
    HWR_GetSurfaceAndPitch(Surface4, &surface, &pitch);
    HWR_RenderToggle();
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_SURF_Z_PTR, &surface);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_SURF_Z_PITCH, &pitch);

    C3D_RECT viewport;
    viewport.top = 0;
    viewport.left = 0;
    viewport.right = DDrawSurfaceWidth - 1;
    viewport.bottom = DDrawSurfaceHeight - 1;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_SURF_VPORT, &viewport);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_SURF_SCISSOR, &viewport);
    LOG_INFO("    complete");
    return 1;
}

void HWR_InitialiseHardware()
{
    int32_t i;
    int32_t tmp;
    HRESULT result;

    LOG_INFO("InitialiseHardware:");

    IsHardwareRenderer = 0;

    for (i = 0; i < MAX_TEXTPAGES; i++) {
        ATITextureMap[i] = NULL;
        TextureSurfaces[i] = NULL;
    }

    result = IDirectDraw_SetCooperativeLevel(
        DDraw, TombHWND, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    HWR_CheckError(result);

    if (!HWR_SetHardwareVideoMode()) {
        return;
    }

    IsHardwareRenderer = 1;

    tmp = C3D_EV_VTCF;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_VERTEX_TYPE, &tmp);
    tmp = C3D_EPRIM_TRI;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_PRIM_TYPE, &tmp);
    tmp = C3D_ESH_SMOOTH;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_SHADE_MODE, &tmp);
    tmp = C3D_ETL_MODULATE;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_TMAP_LIGHT, &tmp);
    tmp = C3D_ETEXOP_CHROMAKEY;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_TMAP_TEXOP, &tmp);
    tmp = C3D_ETFILT_MINPNT_MAGPNT;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_TMAP_FILTER, &tmp);
    tmp = FALSE;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_FOG_EN, &tmp);
    tmp = TRUE;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_DITHER_EN, &tmp);
    tmp = C3D_EZCMP_LEQUAL;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_Z_CMP_FNC, &tmp);
    tmp = C3D_EZMODE_TESTON_WRITEZ;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_Z_MODE, &tmp);
    tmp = C3D_EPF_RGB1555;
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_SURF_DRAW_PF, &tmp);

    LOG_INFO("    Detected %dk video memory", 4096);
    // NOTE: skipped dead code related to caching textures
    LOG_INFO("    Complete, hardware ready");
}

void HWR_ShutdownHardware()
{
    LOG_INFO("ShutdownHardware:");
    IsHardwareRenderer = 0;
    LOG_INFO("    complete");
}

void HWR_PrepareFMV()
{
    LOG_INFO("HardwarePrepareFMV");
    HWR_ClearSurfaceDepth();
    HWR_ReleaseSurfaces();
}

void HWR_FMVDone()
{
    LOG_INFO("HardwareFMVDone");
    HWR_SetHardwareVideoMode();
}

void HWR_FMVInit()
{
    DDSURFACEDESC surface_desc;
    HRESULT result;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_PRIMARYSURFACE;
    result = IDirectDraw2_CreateSurface(DDraw, &surface_desc, &Surface1, NULL);
    HWR_CheckError(result);

    HWR_ClearSurface(Surface1);
    IDirectDrawSurface_Release(Surface1);
    Surface1 = NULL;
}

void HWR_SetupRenderContextAndRender()
{
    HWR_RenderBegin();
    ATI3DCIF_ContextSetState(
        ATIRenderContext, C3D_ERS_SURF_DRAW_PTR, &Surface2DrawPtr);
    int32_t perspective =
        (RenderSettings & RSF_PERSPECTIVE) ? C3D_ETPC_THREE : C3D_ETPC_NONE;
    int32_t filter = (RenderSettings & RSF_BILINEAR)
        ? C3D_ETFILT_MIN2BY2_MAG2BY2
        : C3D_ETFILT_MINPNT_MAGPNT;
    ATI3DCIF_ContextSetState(
        ATIRenderContext, C3D_ERS_TMAP_PERSP_COR, &perspective);
    ATI3DCIF_ContextSetState(ATIRenderContext, C3D_ERS_TMAP_FILTER, &filter);
    HWR_RenderToggle();
}

const int16_t *HWR_InsertObjectG3(const int16_t *obj_ptr, int32_t number)
{
    int32_t i;
    PHD_VBUF *vns[3];
    int32_t color;

    HWR_DisableTextureMode();

    for (i = 0; i < number; i++) {
        vns[0] = &PhdVBuf[*obj_ptr++];
        vns[1] = &PhdVBuf[*obj_ptr++];
        vns[2] = &PhdVBuf[*obj_ptr++];
        color = *obj_ptr++;

        HWR_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
    }

    return obj_ptr;
}

const int16_t *HWR_InsertObjectG4(const int16_t *obj_ptr, int32_t number)
{
    int32_t i;
    PHD_VBUF *vns[4];
    int32_t color;

    HWR_DisableTextureMode();

    for (i = 0; i < number; i++) {
        vns[0] = &PhdVBuf[*obj_ptr++];
        vns[1] = &PhdVBuf[*obj_ptr++];
        vns[2] = &PhdVBuf[*obj_ptr++];
        vns[3] = &PhdVBuf[*obj_ptr++];
        color = *obj_ptr++;

        HWR_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
        HWR_DrawFlatTriangle(vns[2], vns[3], vns[0], color);
    }

    return obj_ptr;
}

const int16_t *HWR_InsertObjectGT3(const int16_t *obj_ptr, int32_t number)
{
    int32_t i;
    PHD_VBUF *vns[3];
    PHD_TEXTURE *tex;

    HWR_EnableTextureMode();

    for (i = 0; i < number; i++) {
        vns[0] = &PhdVBuf[*obj_ptr++];
        vns[1] = &PhdVBuf[*obj_ptr++];
        vns[2] = &PhdVBuf[*obj_ptr++];
        tex = &PhdTextureInfo[*obj_ptr++];

        HWR_DrawTexturedTriangle(
            vns[0], vns[1], vns[2], tex->tpage, &tex->uv[0], &tex->uv[1],
            &tex->uv[2], tex->drawtype);
    }

    return obj_ptr;
}

const int16_t *HWR_InsertObjectGT4(const int16_t *obj_ptr, int32_t number)
{
    int32_t i;
    PHD_VBUF *vns[4];
    PHD_TEXTURE *tex;

    HWR_EnableTextureMode();

    for (i = 0; i < number; i++) {
        vns[0] = &PhdVBuf[*obj_ptr++];
        vns[1] = &PhdVBuf[*obj_ptr++];
        vns[2] = &PhdVBuf[*obj_ptr++];
        vns[3] = &PhdVBuf[*obj_ptr++];
        tex = &PhdTextureInfo[*obj_ptr++];

        HWR_DrawTexturedQuad(
            vns[0], vns[1], vns[2], vns[3], tex->tpage, &tex->uv[0],
            &tex->uv[1], &tex->uv[2], &tex->uv[3], tex->drawtype);
    }

    return obj_ptr;
}

void HWR_DrawFlatTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int32_t color)
{
    int32_t vertex_count;
    C3D_VTCF vertices[8];
    float r;
    float g;
    float b;
    float light;
    float divisor;

    if (!((vn3->clip & vn2->clip & vn1->clip) == 0 && vn1->clip >= 0
          && vn2->clip >= 0 && vn3->clip >= 0
          && (vn1->ys - vn2->ys) * (vn3->xs - vn2->xs)
                  - (vn3->ys - vn2->ys) * (vn1->xs - vn2->xs)
              >= 0)) {
        return;
    }

    r = GamePalette[color].r;
    g = GamePalette[color].g;
    b = GamePalette[color].b;

    if (IsShadeEffect) {
        r *= 0.6f;
        g *= 0.7f;
    }

    divisor = (1.0f / T1MConfig.brightness) * 1024.0f;

    light = (8192.0f - vn1->g) / divisor;
    vertices[0].x = vn1->xs;
    vertices[0].y = vn1->ys;
    vertices[0].z = vn1->zv * 0.0001f;
    vertices[0].r = r * light;
    vertices[0].g = g * light;
    vertices[0].b = b * light;

    light = (8192.0f - vn2->g) / divisor;
    vertices[1].x = vn2->xs;
    vertices[1].y = vn2->ys;
    vertices[1].z = vn2->zv * 0.0001f;
    vertices[1].r = r * light;
    vertices[1].g = g * light;
    vertices[1].b = b * light;

    light = (8192.0f - vn3->g) / divisor;
    vertices[2].x = vn3->xs;
    vertices[2].y = vn3->ys;
    vertices[2].z = vn3->zv * 0.0001f;
    vertices[2].r = r * light;
    vertices[2].g = g * light;
    vertices[2].b = b * light;

    vertex_count = 3;
    if (vn1->clip || vn2->clip || vn3->clip) {
        vertex_count = HWR_ClipVertices(vertex_count, vertices);
    }
    if (!vertex_count) {
        return;
    }

    HWR_RenderTriangleStrip(vertices, vertex_count);
}

void HWR_DrawTexturedTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int16_t tpage, PHD_UV *uv1,
    PHD_UV *uv2, PHD_UV *uv3, uint16_t textype)
{
    int32_t i;
    int32_t vertex_count;
    C3D_VTCF vertices[8];
    POINT_INFO points[3];
    PHD_VBUF *src_vbuf[4];
    PHD_UV *src_uv[4];
    float multiplier;

    multiplier = 0.0625f * T1MConfig.brightness;

    src_vbuf[0] = vn1;
    src_vbuf[1] = vn2;
    src_vbuf[2] = vn3;

    src_uv[0] = uv1;
    src_uv[1] = uv2;
    src_uv[2] = uv3;

    if (vn3->clip & vn2->clip & vn1->clip) {
        return;
    }

    if (vn1->clip >= 0 && vn2->clip >= 0 && vn3->clip >= 0) {
        if ((vn1->ys - vn2->ys) * (vn3->xs - vn2->xs)
                - (vn3->ys - vn2->ys) * (vn1->xs - vn2->xs)
            < 0) {
            return;
        }

        for (i = 0; i < 3; i++) {
            vertices[i].x = src_vbuf[i]->xs;
            vertices[i].y = src_vbuf[i]->ys;
            vertices[i].z = src_vbuf[i]->zv * 0.0001f;

            vertices[i].w = 65536.0f / src_vbuf[i]->zv;
            vertices[i].s = ((src_uv[i]->u1 & 0xFF00) + 127) * 0.00390625f
                * vertices[i].w * 0.00390625f;
            vertices[i].t = ((src_uv[i]->v1 & 0xFF00) + 127) * 0.00390625f
                * vertices[i].w * 0.00390625f;

            vertices[i].r = vertices[i].g = vertices[i].b =
                (8192.0f - src_vbuf[i]->g) * multiplier;

            if (IsShadeEffect) {
                vertices[i].r *= 0.6f;
                vertices[i].g *= 0.7f;
            }
        }

        vertex_count = 3;
        if (vn1->clip || vn2->clip || vn3->clip) {
            vertex_count = HWR_ClipVertices2(vertex_count, vertices);
        }
    } else {
        if (!phd_VisibleZClip(vn1, vn2, vn3)) {
            return;
        }

        for (i = 0; i < 3; i++) {
            points[i].xv = src_vbuf[i]->xv;
            points[i].yv = src_vbuf[i]->yv;
            points[i].zv = src_vbuf[i]->zv;
            points[i].xs = src_vbuf[i]->xs;
            points[i].ys = src_vbuf[i]->ys;
            points[i].g = src_vbuf[i]->g;
            points[i].u = ((src_uv[i]->u1 & 0xFF00) + 127) * 0.00390625f;
            points[i].v = ((src_uv[i]->v1 & 0xFF00) + 127) * 0.00390625f;
        }

        vertex_count = HWR_ZedClipper(3, points, vertices);
        if (!vertex_count) {
            return;
        }
        vertex_count = HWR_ClipVertices2(vertex_count, vertices);
    }

    if (!vertex_count) {
        return;
    }

    if (HWR_TextureLoaded[tpage]) {
        HWR_EnableTextureMode();
        HWR_SelectTexture(tpage);
    }

    HWR_RenderTriangleStrip(vertices, vertex_count);
}

void HWR_DrawTexturedQuad(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, PHD_VBUF *vn4, uint16_t tpage,
    PHD_UV *uv1, PHD_UV *uv2, PHD_UV *uv3, PHD_UV *uv4, uint16_t textype)
{
    int32_t i;
    float multiplier;
    C3D_VTCF vertices[4];
    PHD_VBUF *src_vbuf[4];
    PHD_UV *src_uv[4];

    if (vn4->clip | vn3->clip | vn2->clip | vn1->clip) {
        if ((vn4->clip & vn3->clip & vn2->clip & vn1->clip)) {
            return;
        }

        if (vn1->clip >= 0 && vn2->clip >= 0 && vn3->clip >= 0
            && vn4->clip >= 0) {
            if ((vn1->ys - vn2->ys) * (vn3->xs - vn2->xs)
                    - (vn3->ys - vn2->ys) * (vn1->xs - vn2->xs)
                < 0) {
                return;
            }
        } else if (!phd_VisibleZClip(vn1, vn2, vn3)) {
            return;
        }

        HWR_DrawTexturedTriangle(vn1, vn2, vn3, tpage, uv1, uv2, uv3, textype);
        HWR_DrawTexturedTriangle(vn3, vn4, vn1, tpage, uv3, uv4, uv1, textype);
        return;
    }

    if ((vn1->ys - vn2->ys) * (vn3->xs - vn2->xs)
            - (vn3->ys - vn2->ys) * (vn1->xs - vn2->xs)
        < 0) {
        return;
    }

    multiplier = 0.0625f * T1MConfig.brightness;

    src_vbuf[0] = vn2;
    src_vbuf[1] = vn1;
    src_vbuf[2] = vn3;
    src_vbuf[3] = vn4;

    src_uv[0] = uv2;
    src_uv[1] = uv1;
    src_uv[2] = uv3;
    src_uv[3] = uv4;

    for (i = 0; i < 4; i++) {
        vertices[i].x = src_vbuf[i]->xs;
        vertices[i].y = src_vbuf[i]->ys;
        vertices[i].z = src_vbuf[i]->zv * 0.0001f;

        vertices[i].w = 65536.0f / src_vbuf[i]->zv;
        vertices[i].s = ((src_uv[i]->u1 & 0xFF00) + 127) * 0.00390625f
            * vertices[i].w * 0.00390625f;
        vertices[i].t = ((src_uv[i]->v1 & 0xFF00) + 127) * 0.00390625f
            * vertices[i].w * 0.00390625f;

        vertices[i].r = vertices[i].g = vertices[i].b =
            (8192.0f - src_vbuf[i]->g) * multiplier;

        if (IsShadeEffect) {
            vertices[i].r *= 0.6f;
            vertices[i].g *= 0.7f;
        }
    }

    if (HWR_TextureLoaded[tpage]) {
        HWR_EnableTextureMode();
        HWR_SelectTexture(tpage);
    }

    ATI3DCIF_RenderPrimStrip(vertices, 4);

    // NOTE: original .exe has some additional logic for the case when
    // the requested texture page was not loaded.
}

int32_t
HWR_ZedClipper(int32_t vertex_count, POINT_INFO *pts, C3D_VTCF *vertices)
{
    int32_t i;
    int32_t count;
    POINT_INFO *pts0;
    POINT_INFO *pts1;
    C3D_VTCF *v;
    float clip;
    float near_z;
    float persp_o_near_z;
    float multiplier;

    multiplier = 0.0625f * T1MConfig.brightness;
    near_z = PhdNearZ;
    persp_o_near_z = PhdPersp / near_z;

    v = &vertices[0];
    pts0 = &pts[vertex_count - 1];
    for (i = 0; i < vertex_count; i++) {
        pts1 = pts0;
        pts0 = &pts[i];
        if (near_z > pts1->zv) {
            if (near_z > pts0->zv) {
                continue;
            }

            clip = (near_z - pts0->zv) / (pts1->zv - pts0->zv);
            v->x = ((pts1->xv - pts0->xv) * clip + pts0->xv) * persp_o_near_z
                + PhdCenterX;
            v->y = ((pts1->yv - pts0->yv) * clip + pts0->yv) * persp_o_near_z
                + PhdCenterY;
            v->z = near_z * 0.0001f;

            v->w = 65536.0f / near_z;
            v->s = v->w * ((pts1->u - pts0->u) * clip + pts0->u) * 0.00390625f;
            v->t = v->w * ((pts1->v - pts0->v) * clip + pts0->v) * 0.00390625f;

            v->r = v->g = v->b =
                (8192.0f - ((pts1->g - pts0->g) * clip + pts0->g)) * multiplier;

            if (IsShadeEffect) {
                v->r *= 0.6f;
                v->g *= 0.7f;
            }
            v++;
        }

        if (near_z > pts0->zv) {
            clip = (near_z - pts0->zv) / (pts1->zv - pts0->zv);
            v->x = ((pts1->xv - pts0->xv) * clip + pts0->xv) * persp_o_near_z
                + PhdCenterX;
            v->y = ((pts1->yv - pts0->yv) * clip + pts0->yv) * persp_o_near_z
                + PhdCenterY;
            v->z = near_z * 0.0001f;

            v->w = 65536.0f / near_z;
            v->s = v->w * ((pts1->u - pts0->u) * clip + pts0->u) * 0.00390625f;
            v->t = v->w * ((pts1->v - pts0->v) * clip + pts0->v) * 0.00390625f;

            v->r = v->g = v->b =
                (8192.0f - ((pts1->g - pts0->g) * clip + pts0->g)) * multiplier;
            if (IsShadeEffect) {
                v->r *= 0.6f;
                v->g *= 0.7f;
            }
            v++;
        } else {
            v->x = pts0->xs;
            v->y = pts0->ys;
            v->z = pts0->zv * 0.0001f;

            v->w = 65536.0f / pts0->zv;
            v->s = pts0->u * v->w * 0.00390625f;
            v->t = pts0->v * v->w * 0.00390625f;

            v->r = v->g = v->b = (8192.0f - pts0->g) * multiplier;

            if (IsShadeEffect) {
                v->r *= 0.6f;
                v->g *= 0.7f;
            }
            v++;
        }
    }

    count = v - vertices;
    return count < 3 ? 0 : count;
}

void HWR_InitPolyList()
{
    HWR_RenderBegin();

    // NOTE: original .exe has some additional logic for rendering
    // polygons whose textures were not loaded at the draw time.

    HWR_LightningCount = 0;
}

void HWR_OutputPolyList()
{
    int i;

    // NOTE: original .exe has some additional logic for rendering
    // polygons whose textures were not loaded at the draw time.

    for (i = 0; i < HWR_LightningCount; i++) {
        HWR_RenderLightningSegment(
            HWR_LightningTable[i].x1, HWR_LightningTable[i].y1,
            HWR_LightningTable[i].z1, HWR_LightningTable[i].thickness1,
            HWR_LightningTable[i].x2, HWR_LightningTable[i].y2,
            HWR_LightningTable[i].z2, HWR_LightningTable[i].thickness2);
    }
    HWR_RenderEnd();
}

void T1MInjectSpecificHWR()
{
    INJECT(0x004077D0, HWR_CheckError);
    INJECT(0x00407827, HWR_RenderBegin);
    INJECT(0x0040783B, HWR_RenderEnd);
    INJECT(0x00407862, HWR_RenderToggle);
    INJECT(0x0040787C, HWR_GetSurfaceAndPitch);
    INJECT(0x0040795F, HWR_SetupRenderContextAndRender);
    INJECT(0x004079E9, HWR_FlipPrimaryBuffer);
    INJECT(0x00407A49, HWR_ClearSurface);
    INJECT(0x00407A91, HWR_ReleaseSurfaces);
    INJECT(0x00407BD2, HWR_SetHardwareVideoMode);
    INJECT(0x00408005, HWR_InitialiseHardware);
    INJECT(0x00408323, HWR_ShutdownHardware);
    INJECT(0x0040834C, HWR_PrepareFMV);
    INJECT(0x00408368, HWR_FMVDone);
    INJECT(0x0040837F, HWR_FMVInit);
    INJECT(0x004087EA, HWR_SetPalette);
    INJECT(0x004089F4, HWR_SwitchResolution);
    INJECT(0x00408A70, HWR_DumpScreen);
    INJECT(0x00408AC7, HWR_ClearSurfaceDepth);
    INJECT(0x00408B2C, HWR_BlitSurface);
    INJECT(0x00408B85, HWR_CopyPicture);
    INJECT(0x00408C3A, HWR_DownloadPicture);
    INJECT(0x00408E32, HWR_FadeWait);
    INJECT(0x00408E6D, HWR_RenderTriangleStrip);
    INJECT(0x00408FF0, HWR_SelectTexture);
    INJECT(0x0040904D, HWR_ClipVertices);
    INJECT(0x00409C0F, HWR_DrawFlatTriangle);
    INJECT(0x00409F44, HWR_InsertObjectG4);
    INJECT(0x0040A01D, HWR_InsertObjectG3);
    INJECT(0x0040A0C4, HWR_ZedClipper);
    INJECT(0x0040A6B1, HWR_ClipVertices2);
    INJECT(0x0040B510, HWR_DrawTexturedTriangle);
    INJECT(0x0040BBE2, HWR_DrawTexturedQuad);
    INJECT(0x0040C25A, HWR_InsertObjectGT4);
    INJECT(0x0040C34E, HWR_InsertObjectGT3);
    INJECT(0x0040C425, HWR_DrawSprite);
    INJECT(0x0040C7EE, HWR_Draw2DLine);
    INJECT(0x0040C8E7, HWR_DrawTranslucentQuad);
    INJECT(0x0040CADB, HWR_PrintShadow);
    INJECT(0x0040CC5D, HWR_RenderLightningSegment);
    INJECT(0x0040D056, HWR_DrawLightningSegment);
    INJECT(0x0040D0F7, HWR_InitPolyList);
    INJECT(0x0040D2E0, HWR_OutputPolyList);
}
