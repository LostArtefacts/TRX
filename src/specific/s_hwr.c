#include "specific/s_hwr.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "game/screen.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "log.h"
#include "specific/s_ati.h"
#include "specific/s_shell.h"

#include "ati3dcif/Interop.hpp"
#include "ddraw/Interop.hpp"
#include "glrage/Interop.hpp"

#include <assert.h>

#define HWR_CheckError(result)                                                 \
    {                                                                          \
        if (result != DD_OK) {                                                 \
            LOG_ERROR("DirectDraw error code %x", result);                     \
            S_Shell_ExitSystem("Fatal DirectDraw error!");                     \
        }                                                                      \
    }

static C3D_HTX m_ATITextureMap[MAX_TEXTPAGES];
static C3D_HTXPAL m_ATITexturePalette;
static C3D_PALETTENTRY m_ATIPalette[256];
static C3D_COLOR m_ATIChromaKey;

static bool m_IsPaletteActive = false;
static bool m_IsRendering = false;
static bool m_IsRenderingOld = false;
static bool m_IsTextureMode = false;
static int32_t m_SelectedTexture = -1;
static bool m_TextureLoaded[MAX_TEXTPAGES] = { false };
static RGBF m_WaterColor = { 0 };

static int32_t m_DDrawSurfaceWidth = 0;
static int32_t m_DDrawSurfaceHeight = 0;
static float m_DDrawSurfaceMinX = 0.0f;
static float m_DDrawSurfaceMinY = 0.0f;
static float m_DDrawSurfaceMaxX = 0.0f;
static float m_DDrawSurfaceMaxY = 0.0f;
static LPDIRECTDRAWSURFACE m_PrimarySurface = NULL;
static LPDIRECTDRAWSURFACE m_BackSurface = NULL;
static LPDIRECTDRAWSURFACE m_PictureSurface = NULL;
static LPDIRECTDRAWSURFACE m_TextureSurfaces[MAX_TEXTPAGES] = { NULL };

static void HWR_EnableTextureMode(void);
static void HWR_DisableTextureMode(void);
static void HWR_ApplyWaterEffect(float *r, float *g, float *b);

static void HWR_EnableTextureMode(void)
{
    if (m_IsTextureMode) {
        return;
    }

    m_IsTextureMode = true;
    BOOL enable = TRUE;
    ATI3DCIF_SetState(C3D_ERS_TMAP_EN, &enable);
}

static void HWR_DisableTextureMode(void)
{
    if (!m_IsTextureMode) {
        return;
    }

    m_IsTextureMode = false;
    BOOL enable = FALSE;
    ATI3DCIF_SetState(C3D_ERS_TMAP_EN, &enable);
}

static void HWR_ApplyWaterEffect(float *r, float *g, float *b)
{
    if (g_IsShadeEffect) {
        *r *= m_WaterColor.r;
        *g *= m_WaterColor.g;
        *b *= m_WaterColor.b;
    }
}

void HWR_SetWaterColor(const RGBF *color)
{
    m_WaterColor.r = color->r;
    m_WaterColor.g = color->g;
    m_WaterColor.b = color->b;
}

void HWR_RenderBegin()
{
    m_IsRenderingOld = m_IsRendering;
    if (!m_IsRendering) {
        ATI3DCIF_RenderBegin();
        m_IsRendering = true;
    }
}

void HWR_RenderEnd()
{
    m_IsRenderingOld = m_IsRendering;
    if (m_IsRendering) {
        ATI3DCIF_RenderEnd();
        m_IsRendering = false;
    }
}

void HWR_RenderToggle()
{
    if (m_IsRenderingOld) {
        HWR_RenderBegin();
    } else {
        HWR_RenderEnd();
    }
}

void HWR_ClearSurface(LPDIRECTDRAWSURFACE surface)
{
    HRESULT result =
        MyIDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL);
    HWR_CheckError(result);
}

void HWR_ReleaseSurfaces()
{
    int i;
    HRESULT result;

    if (m_PrimarySurface) {
        HWR_ClearSurface(m_PrimarySurface);
        HWR_ClearSurface(m_BackSurface);

        result = MyIDirectDrawSurface_Release(m_PrimarySurface);
        HWR_CheckError(result);
        m_PrimarySurface = NULL;
        m_BackSurface = NULL;
    }

    for (i = 0; i < MAX_TEXTPAGES; i++) {
        if (m_TextureSurfaces[i]) {
            result = MyIDirectDrawSurface_Release(m_TextureSurfaces[i]);
            HWR_CheckError(result);
            m_TextureSurfaces[i] = NULL;
        }
    }

    if (m_PictureSurface) {
        result = MyIDirectDrawSurface_Release(m_PictureSurface);
        HWR_CheckError(result);
        m_PictureSurface = NULL;
    }
}

void HWR_SetPalette()
{
    int32_t i;

    LOG_INFO("PaletteSetHardware:");

    m_ATIPalette[0].r = 0;
    m_ATIPalette[0].g = 0;
    m_ATIPalette[0].b = 0;
    m_ATIPalette[0].flags = C3D_LOAD_PALETTE_ENTRY;

    for (i = 1; i < 256; i++) {
        if (g_GamePalette[i].r || g_GamePalette[i].g || g_GamePalette[i].b) {
            m_ATIPalette[i].r = 4 * g_GamePalette[i].r;
            m_ATIPalette[i].g = 4 * g_GamePalette[i].g;
            m_ATIPalette[i].b = 4 * g_GamePalette[i].b;
        } else {
            m_ATIPalette[i].r = 1;
            m_ATIPalette[i].g = 1;
            m_ATIPalette[i].b = 1;
        }
        m_ATIPalette[i].flags = C3D_LOAD_PALETTE_ENTRY;
    }

    m_ATIChromaKey.r = 0;
    m_ATIChromaKey.g = 0;
    m_ATIChromaKey.b = 0;
    m_ATIChromaKey.a = 0;

    m_IsPaletteActive = true;
    LOG_INFO("    complete");
}

void HWR_DumpScreen()
{
    HWR_FlipPrimaryBuffer();
    m_SelectedTexture = -1;
}

void HWR_ClearBackBuffer()
{
    HWR_RenderEnd();
    HWR_ClearSurface(m_BackSurface);
    HWR_RenderToggle();
}

void HWR_FlipPrimaryBuffer()
{
    HWR_RenderEnd();
    HRESULT result = MyIDirectDrawSurface_Flip(m_PrimarySurface);
    HWR_CheckError(result);
    HWR_RenderToggle();

    HWR_SetupRenderContextAndRender();
}

void HWR_BlitSurface(LPDIRECTDRAWSURFACE source, LPDIRECTDRAWSURFACE target)
{
    RECT rect;
    SetRect(&rect, 0, 0, m_DDrawSurfaceWidth, m_DDrawSurfaceHeight);
    HRESULT result = MyIDirectDrawSurface_Blt(target, &rect, source, &rect, 0);
    HWR_CheckError(result);
}

void HWR_CopyFromPicture()
{
    LOG_INFO("CopyPictureHardware:");

    HRESULT result;

    if (!m_PictureSurface) {
        DDSURFACEDESC surface_desc;
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.dwWidth = m_DDrawSurfaceWidth;
        surface_desc.dwHeight = m_DDrawSurfaceHeight;
        result = MyIDirectDraw2_CreateSurface(
            g_DDraw, &surface_desc, &m_PictureSurface);
        HWR_CheckError(result);
    }

    HWR_RenderEnd();
    HWR_BlitSurface(m_BackSurface, m_PictureSurface);
    HWR_RenderToggle();
    LOG_INFO("    complete");
}

void HWR_CopyToPicture()
{
    HWR_ClearBackBuffer();
    HWR_RenderEnd();
    HWR_BlitSurface(m_PictureSurface, m_BackSurface);
    HWR_RenderToggle();
}

void HWR_DownloadPicture(const PICTURE *pic)
{
    LOG_INFO("DownloadPictureHardware:");

    LPDIRECTDRAWSURFACE picture_surface = NULL;
    DDSURFACEDESC surface_desc;
    HRESULT result;

    // first, download the picture directly to a temporary surface
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.dwWidth = pic->width;
    surface_desc.dwHeight = pic->height;
    result =
        MyIDirectDraw2_CreateSurface(g_DDraw, &surface_desc, &picture_surface);
    HWR_CheckError(result);

    memset(&surface_desc, 0, sizeof(surface_desc));

    result = MyIDirectDrawSurface2_Lock(picture_surface, &surface_desc);
    HWR_CheckError(result);

    uint32_t *output_ptr = surface_desc.lpSurface;
    RGB888 *input_ptr = pic->data;
    for (int i = 0; i < pic->width * pic->height; i++) {
        uint8_t r = input_ptr->r;
        uint8_t g = input_ptr->g;
        uint8_t b = input_ptr->b;
        input_ptr++;
        *output_ptr++ = b | (g << 8) | (r << 16);
    }

    result =
        MyIDirectDrawSurface2_Unlock(picture_surface, surface_desc.lpSurface);
    HWR_CheckError(result);

    if (!m_PictureSurface) {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.dwWidth = m_DDrawSurfaceWidth;
        surface_desc.dwHeight = m_DDrawSurfaceHeight;
        result = MyIDirectDraw2_CreateSurface(
            g_DDraw, &surface_desc, &m_PictureSurface);
        HWR_CheckError(result);
    }

    int32_t target_width = m_DDrawSurfaceWidth;
    int32_t target_height = m_DDrawSurfaceHeight;
    int32_t source_width = pic->width;
    int32_t source_height = pic->height;

    // keep aspect ratio and fit inside, adding black bars on the sides
    const float source_ratio = source_width / (float)source_height;
    const float target_ratio = target_width / (float)target_height;
    int32_t new_width = source_ratio < target_ratio
        ? target_height * source_ratio
        : target_width;
    int32_t new_height = source_ratio < target_ratio
        ? target_height
        : target_width / source_ratio;

    RECT source_rect;
    source_rect.left = 0;
    source_rect.top = 0;
    source_rect.right = pic->width;
    source_rect.bottom = pic->height;
    RECT target_rect;
    target_rect.left = (target_width - new_width) / 2;
    target_rect.top = (target_height - new_height) / 2;
    target_rect.right = target_rect.left + new_width;
    target_rect.bottom = target_rect.top + new_height;

    result = MyIDirectDrawSurface_Blt(
        m_PictureSurface, &target_rect, picture_surface, &source_rect, 0);
    HWR_CheckError(result);

    MyIDirectDrawSurface_Release(picture_surface);

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
    if (tex_num == m_SelectedTexture) {
        return;
    }

    if (!m_TextureLoaded[tex_num]) {
        return;
    }

    if (!m_ATITextureMap[tex_num]) {
        LOG_ERROR("ERROR: Attempt to select unloaded texture");
        return;
    }

    if (ATI3DCIF_SetState(C3D_ERS_TMAP_SELECT, &m_ATITextureMap[tex_num])) {
        LOG_ERROR("    Texture error");
        return;
    }

    m_SelectedTexture = tex_num;
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

    multiplier = 0.0625f * g_Config.brightness;

    sprite = &g_PhdSpriteInfo[sprnum];
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
    if (x1 < 0 || y1 < 0 || x2 > ViewPort_GetWidth()
        || y2 > ViewPort_GetHeight()) {
        vertex_count = HWR_ClipVertices2(vertex_count, vertices);
    }

    if (!vertex_count) {
        return;
    }

    if (m_TextureLoaded[sprite->tpage]) {
        HWR_EnableTextureMode();
        HWR_SelectTexture(sprite->tpage);
        HWR_RenderTriangleStrip(vertices, vertex_count);
    } else {
        HWR_DisableTextureMode();
        HWR_RenderTriangleStrip(vertices, vertex_count);
    }
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
    ATI3DCIF_SetState(C3D_ERS_PRIM_TYPE, &prim_type);

    HWR_DisableTextureMode();

    ATI3DCIF_RenderPrimList((C3D_VLIST)v_list, 2);

    prim_type = C3D_EPRIM_TRI;
    ATI3DCIF_SetState(C3D_ERS_PRIM_TYPE, &prim_type);
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

    HWR_DisableTextureMode();

    HWR_RenderTriangleStrip(vertices, 4);
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

    HWR_DisableTextureMode();

    int32_t alpha_src = C3D_EASRC_SRCALPHA;
    int32_t alpha_dst = C3D_EADST_INVSRCALPHA;
    ATI3DCIF_SetState(C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_SetState(C3D_ERS_ALPHA_DST, &alpha_dst);

    HWR_RenderTriangleStrip(vertices, 4);

    alpha_src = C3D_EASRC_ONE;
    alpha_dst = C3D_EADST_ZERO;
    ATI3DCIF_SetState(C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_SetState(C3D_ERS_ALPHA_DST, &alpha_dst);
}

void HWR_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2)
{
    C3D_VTCF vertices[4 * HWR_CLIP_VERTCOUNT_SCALE];

    HWR_DisableTextureMode();

    int32_t alpha_src = C3D_EASRC_SRCALPHA;
    int32_t alpha_dst = C3D_EADST_INVSRCALPHA;
    ATI3DCIF_SetState(C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_SetState(C3D_ERS_ALPHA_DST, &alpha_dst);
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

    alpha_src = C3D_EASRC_ONE;
    alpha_dst = C3D_EADST_ZERO;
    ATI3DCIF_SetState(C3D_ERS_ALPHA_SRC, &alpha_src);
    ATI3DCIF_SetState(C3D_ERS_ALPHA_DST, &alpha_dst);
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
    ATI3DCIF_SetState(C3D_ERS_ALPHA_SRC, &tmp);
    tmp = C3D_EADST_INVSRCALPHA;
    ATI3DCIF_SetState(C3D_ERS_ALPHA_DST, &tmp);
    HWR_RenderTriangleStrip(vertices, vertex_count);
    tmp = C3D_EASRC_ONE;
    ATI3DCIF_SetState(C3D_ERS_ALPHA_SRC, &tmp);
    tmp = C3D_EADST_ZERO;
    ATI3DCIF_SetState(C3D_ERS_ALPHA_DST, &tmp);
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

        if (v2->x < m_DDrawSurfaceMinX) {
            if (l->x < m_DDrawSurfaceMinX) {
                continue;
            }
            scale = (m_DDrawSurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = m_DDrawSurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        } else if (v2->x > m_DDrawSurfaceMaxX) {
            if (l->x > m_DDrawSurfaceMaxX) {
                continue;
            }
            scale = (m_DDrawSurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = m_DDrawSurfaceMaxX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        }

        if (l->x < m_DDrawSurfaceMinX) {
            scale = (m_DDrawSurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = m_DDrawSurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &vertices[++j];
        } else if (l->x > m_DDrawSurfaceMaxX) {
            scale = (m_DDrawSurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = m_DDrawSurfaceMaxX;
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

        if (v2->y < m_DDrawSurfaceMinY) {
            if (l->y < m_DDrawSurfaceMinY) {
                continue;
            }
            scale = (m_DDrawSurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_DDrawSurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        } else if (v2->y > m_DDrawSurfaceMaxY) {
            if (l->y > m_DDrawSurfaceMaxY) {
                continue;
            }
            scale = (m_DDrawSurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_DDrawSurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        }

        if (l->y < m_DDrawSurfaceMinY) {
            scale = (m_DDrawSurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_DDrawSurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->a = (v2->a - l->a) * scale + l->a;
            v1 = &source[++j];
        } else if (l->y > m_DDrawSurfaceMaxY) {
            scale = (m_DDrawSurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_DDrawSurfaceMaxY;
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

        if (v2->x < m_DDrawSurfaceMinX) {
            if (l->x < m_DDrawSurfaceMinX) {
                continue;
            }
            scale = (m_DDrawSurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = m_DDrawSurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        } else if (v2->x > m_DDrawSurfaceMaxX) {
            if (l->x > m_DDrawSurfaceMaxX) {
                continue;
            }
            scale = (m_DDrawSurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = m_DDrawSurfaceMaxX;
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

        if (l->x < m_DDrawSurfaceMinX) {
            scale = (m_DDrawSurfaceMinX - l->x) / (v2->x - l->x);
            v1->x = m_DDrawSurfaceMinX;
            v1->y = (v2->y - l->y) * scale + l->y;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &vertices[++j];
        } else if (l->x > m_DDrawSurfaceMaxX) {
            scale = (m_DDrawSurfaceMaxX - l->x) / (v2->x - l->x);
            v1->x = m_DDrawSurfaceMaxX;
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

        if (v2->y < m_DDrawSurfaceMinY) {
            if (l->y < m_DDrawSurfaceMinY) {
                continue;
            }
            scale = (m_DDrawSurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_DDrawSurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        } else if (v2->y > m_DDrawSurfaceMaxY) {
            if (l->y > m_DDrawSurfaceMaxY) {
                continue;
            }
            scale = (m_DDrawSurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_DDrawSurfaceMaxY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        }

        if (l->y < m_DDrawSurfaceMinY) {
            scale = (m_DDrawSurfaceMinY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_DDrawSurfaceMinY;
            v1->z = (v2->z - l->z) * scale + l->z;
            v1->r = (v2->r - l->r) * scale + l->r;
            v1->g = (v2->g - l->g) * scale + l->g;
            v1->b = (v2->b - l->b) * scale + l->b;
            v1->w = (v2->w - l->w) * scale + l->w;
            v1->s = (v2->s - l->s) * scale + l->s;
            v1->t = (v2->t - l->t) * scale + l->t;
            v1 = &source[++j];
        } else if (l->y > m_DDrawSurfaceMaxY) {
            scale = (m_DDrawSurfaceMaxY - l->y) / (v2->y - l->y);
            v1->x = (v2->x - l->x) * scale + l->x;
            v1->y = m_DDrawSurfaceMaxY;
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
    HWR_ClearBackBuffer();
    HWR_DumpScreen();
}

void HWR_SwitchResolution()
{
    HWR_SetHardwareVideoMode();
    Screen_SetupSize();
}

void HWR_SetHardwareVideoMode()
{
    DDSURFACEDESC surface_desc;
    HRESULT result;

    LOG_INFO("SetHardwareVideoMode:");
    HWR_ReleaseSurfaces();

    m_DDrawSurfaceWidth = Screen_GetResWidth();
    m_DDrawSurfaceHeight = Screen_GetResHeight();
    m_DDrawSurfaceMinX = 0.0f;
    m_DDrawSurfaceMinY = 0.0f;
    m_DDrawSurfaceMaxX = Screen_GetResWidth() - 1.0f;
    m_DDrawSurfaceMaxY = Screen_GetResHeight() - 1.0f;

    LOG_INFO(
        "    Switching to %dx%d", m_DDrawSurfaceWidth, m_DDrawSurfaceHeight);
    result = MyIDirectDraw_SetDisplayMode(
        g_DDraw, m_DDrawSurfaceWidth, m_DDrawSurfaceHeight);
    HWR_CheckError(result);

    LOG_INFO("    Allocating front/back buffers");
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP;
    surface_desc.dwBackBufferCount = 1;
    result =
        MyIDirectDraw2_CreateSurface(g_DDraw, &surface_desc, &m_PrimarySurface);
    HWR_CheckError(result);
    HWR_ClearSurface(m_PrimarySurface);

    LOG_INFO("    Picking up back buffer");
    DDSCAPS caps = { DDSCAPS_BACKBUFFER };
    result = MyIDirectDrawSurface_GetAttachedSurface(
        m_PrimarySurface, &caps, &m_BackSurface);
    HWR_CheckError(result);

    LOG_INFO("    Creating texture surfaces");
    for (int i = 0; i < MAX_TEXTPAGES; i++) {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        surface_desc.ddpfPixelFormat.dwRGBBitCount = 8;
        surface_desc.dwWidth = 256;
        surface_desc.dwHeight = 256;
        result = MyIDirectDraw2_CreateSurface(
            g_DDraw, &surface_desc, &m_TextureSurfaces[i]);
        HWR_CheckError(result);
    }

    HWR_SetupRenderContextAndRender();

    LOG_INFO("    complete");
}

void HWR_SetViewport(int width, int height)
{
    GLRage_SetWindowSize(width, height);
}

void HWR_SetFullscreen(bool fullscreen)
{
    GLRage_SetFullscreen(fullscreen);
}

bool HWR_Init()
{
    int32_t i;
    int32_t tmp;
    HRESULT result;

    GLRage_Attach(g_TombHWND);
    if (MyDirectDrawCreate(&g_DDraw) != DD_OK) {
        LOG_ERROR("DDraw emulation layer could not be started");
        return false;
    }
    if (!S_ATI_Init()) {
        LOG_ERROR("ATI3DCIF emulation layer could not be started");
        return false;
    }

    LOG_INFO("InitialiseHardware:");

    for (i = 0; i < MAX_TEXTPAGES; i++) {
        m_ATITextureMap[i] = NULL;
        m_TextureSurfaces[i] = NULL;
    }

    HWR_SetHardwareVideoMode();

    tmp = C3D_EV_VTCF;
    ATI3DCIF_SetState(C3D_ERS_VERTEX_TYPE, &tmp);
    tmp = C3D_EPRIM_TRI;
    ATI3DCIF_SetState(C3D_ERS_PRIM_TYPE, &tmp);
    tmp = C3D_ESH_SMOOTH;
    ATI3DCIF_SetState(C3D_ERS_SHADE_MODE, &tmp);
    tmp = C3D_ETL_MODULATE;
    ATI3DCIF_SetState(C3D_ERS_TMAP_LIGHT, &tmp);
    tmp = C3D_ETEXOP_CHROMAKEY;
    ATI3DCIF_SetState(C3D_ERS_TMAP_TEXOP, &tmp);
    tmp = C3D_ETFILT_MINPNT_MAGPNT;
    ATI3DCIF_SetState(C3D_ERS_TMAP_FILTER, &tmp);
    tmp = C3D_EZCMP_LEQUAL;
    ATI3DCIF_SetState(C3D_ERS_Z_CMP_FNC, &tmp);
    tmp = C3D_EZMODE_TESTON_WRITEZ;
    ATI3DCIF_SetState(C3D_ERS_Z_MODE, &tmp);

    LOG_INFO("    Detected %dk video memory", 4096);
    LOG_INFO("    Complete, hardware ready");
    return true;
}

void HWR_Shutdown()
{
    LOG_INFO("ShutdownHardware:");
    LOG_INFO("    complete");
    HWR_ReleaseSurfaces();
    S_ATI_Shutdown();
    if (g_DDraw) {
        MyIDirectDraw_Release(g_DDraw);
        g_DDraw = NULL;
    }
    GLRage_Detach();
}

void HWR_SetupRenderContextAndRender()
{
    HWR_RenderBegin();
    int32_t filter = g_Config.render_flags.bilinear ? C3D_ETFILT_MIN2BY2_MAG2BY2
                                                    : C3D_ETFILT_MINPNT_MAGPNT;
    ATI3DCIF_SetState(C3D_ERS_TMAP_FILTER, &filter);
    HWR_RenderToggle();
}

const int16_t *HWR_InsertObjectG3(const int16_t *obj_ptr, int32_t number)
{
    int32_t i;
    PHD_VBUF *vns[3];
    int32_t color;

    HWR_DisableTextureMode();

    for (i = 0; i < number; i++) {
        vns[0] = &g_PhdVBuf[*obj_ptr++];
        vns[1] = &g_PhdVBuf[*obj_ptr++];
        vns[2] = &g_PhdVBuf[*obj_ptr++];
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
        vns[0] = &g_PhdVBuf[*obj_ptr++];
        vns[1] = &g_PhdVBuf[*obj_ptr++];
        vns[2] = &g_PhdVBuf[*obj_ptr++];
        vns[3] = &g_PhdVBuf[*obj_ptr++];
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
        vns[0] = &g_PhdVBuf[*obj_ptr++];
        vns[1] = &g_PhdVBuf[*obj_ptr++];
        vns[2] = &g_PhdVBuf[*obj_ptr++];
        tex = &g_PhdTextureInfo[*obj_ptr++];

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
        vns[0] = &g_PhdVBuf[*obj_ptr++];
        vns[1] = &g_PhdVBuf[*obj_ptr++];
        vns[2] = &g_PhdVBuf[*obj_ptr++];
        vns[3] = &g_PhdVBuf[*obj_ptr++];
        tex = &g_PhdTextureInfo[*obj_ptr++];

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

    r = g_GamePalette[color].r;
    g = g_GamePalette[color].g;
    b = g_GamePalette[color].b;

    HWR_ApplyWaterEffect(&r, &g, &b);

    divisor = (1.0f / g_Config.brightness) * 1024.0f;

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

    multiplier = 0.0625f * g_Config.brightness;

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

            HWR_ApplyWaterEffect(
                &vertices[i].r, &vertices[i].g, &vertices[i].b);
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

    if (m_TextureLoaded[tpage]) {
        HWR_EnableTextureMode();
        HWR_SelectTexture(tpage);
        HWR_RenderTriangleStrip(vertices, vertex_count);
    } else {
        HWR_DisableTextureMode();
        HWR_RenderTriangleStrip(vertices, vertex_count);
    }
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

    multiplier = 0.0625f * g_Config.brightness;

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

        HWR_ApplyWaterEffect(&vertices[i].r, &vertices[i].g, &vertices[i].b);
    }

    if (m_TextureLoaded[tpage]) {
        HWR_EnableTextureMode();
        HWR_SelectTexture(tpage);
    } else {
        HWR_DisableTextureMode();
    }

    ATI3DCIF_RenderPrimStrip(vertices, 4);
}

int32_t HWR_ZedClipper(
    int32_t vertex_count, POINT_INFO *pts, C3D_VTCF *vertices)
{
    int32_t i;
    int32_t count;
    POINT_INFO *pts0;
    POINT_INFO *pts1;
    C3D_VTCF *v;
    float clip;
    float persp_o_near_z;
    float multiplier;

    multiplier = 0.0625f * g_Config.brightness;
    float near_z = phd_GetNearZ();
    persp_o_near_z = g_PhdPersp / near_z;

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
                + ViewPort_GetCenterX();
            v->y = ((pts1->yv - pts0->yv) * clip + pts0->yv) * persp_o_near_z
                + ViewPort_GetCenterY();
            v->z = near_z * 0.0001f;

            v->w = 65536.0f / near_z;
            v->s = v->w * ((pts1->u - pts0->u) * clip + pts0->u) * 0.00390625f;
            v->t = v->w * ((pts1->v - pts0->v) * clip + pts0->v) * 0.00390625f;

            v->r = v->g = v->b =
                (8192.0f - ((pts1->g - pts0->g) * clip + pts0->g)) * multiplier;
            HWR_ApplyWaterEffect(&v->r, &v->g, &v->b);

            v++;
        }

        if (near_z > pts0->zv) {
            clip = (near_z - pts0->zv) / (pts1->zv - pts0->zv);
            v->x = ((pts1->xv - pts0->xv) * clip + pts0->xv) * persp_o_near_z
                + ViewPort_GetCenterX();
            v->y = ((pts1->yv - pts0->yv) * clip + pts0->yv) * persp_o_near_z
                + ViewPort_GetCenterY();
            v->z = near_z * 0.0001f;

            v->w = 65536.0f / near_z;
            v->s = v->w * ((pts1->u - pts0->u) * clip + pts0->u) * 0.00390625f;
            v->t = v->w * ((pts1->v - pts0->v) * clip + pts0->v) * 0.00390625f;

            v->r = v->g = v->b =
                (8192.0f - ((pts1->g - pts0->g) * clip + pts0->g)) * multiplier;
            HWR_ApplyWaterEffect(&v->r, &v->g, &v->b);

            v++;
        } else {
            v->x = pts0->xs;
            v->y = pts0->ys;
            v->z = pts0->zv * 0.0001f;

            v->w = 65536.0f / pts0->zv;
            v->s = pts0->u * v->w * 0.00390625f;
            v->t = pts0->v * v->w * 0.00390625f;

            v->r = v->g = v->b = (8192.0f - pts0->g) * multiplier;
            HWR_ApplyWaterEffect(&v->r, &v->g, &v->b);

            v++;
        }
    }

    count = v - vertices;
    return count < 3 ? 0 : count;
}

void HWR_InitPolyList()
{
    HWR_RenderBegin();
}

void HWR_OutputPolyList()
{
    HWR_RenderEnd();
}

void HWR_DownloadTextures(int32_t pages)
{
    int i;

    LOG_INFO(
        "DownloadTexturesToHardware: level %d, pages %d", g_CurrentLevel,
        pages);

    if (pages > MAX_TEXTPAGES) {
        S_Shell_ExitSystem("Attempt to download more than texture page limit");
    }

    for (i = 0; i < MAX_TEXTPAGES; i++) {
        if (m_ATITextureMap[i]) {
            if (ATI3DCIF_TextureUnreg(m_ATITextureMap[i])) {
                S_Shell_ExitSystem("ERROR: Could not unregister texture");
            }
            m_ATITextureMap[i] = 0;
        }
        m_TextureLoaded[i] = false;
    }

    if (m_IsPaletteActive) {
        LOG_INFO("    Resetting texture palette handle");
        if (m_ATITexturePalette) {
            if (ATI3DCIF_TexturePaletteDestroy(m_ATITexturePalette)) {
                S_Shell_ExitSystem("ERROR: Cannot release old texture palette");
            }
            m_ATITexturePalette = NULL;
        }
        if (ATI3DCIF_TexturePaletteCreate(
                C3D_ECI_TMAP_8BIT, m_ATIPalette, &m_ATITexturePalette)) {
            S_Shell_ExitSystem("ERROR: Cannot create texture palette");
        }
        m_IsPaletteActive = false;
    }

    for (i = 0; i < pages; i++) {
        DDSURFACEDESC surface_desc;
        HRESULT result;

        memset(&surface_desc, 0, sizeof(surface_desc));
        result =
            MyIDirectDrawSurface2_Lock(m_TextureSurfaces[i], &surface_desc);
        HWR_CheckError(result);

        memcpy(
            surface_desc.lpSurface, g_TexturePagePtrs[i],
            0x10000); // TODO: magic number

        result = MyIDirectDrawSurface2_Unlock(
            m_TextureSurfaces[i], surface_desc.lpSurface);
        HWR_CheckError(result);

        LOG_INFO("    registering");

        C3D_TMAP tmap;
        tmap.u32Size = sizeof(C3D_TMAP);
        tmap.bMipMap = FALSE;
        tmap.apvLevels[0] = (C3D_PVOID)surface_desc.lpSurface;
        tmap.u32MaxMapXSizeLg2 = 8;
        tmap.u32MaxMapYSizeLg2 = 8;
        tmap.eTexFormat = C3D_ETF_CI8;
        tmap.clrTexChromaKey = m_ATIChromaKey;
        tmap.htxpalTexPalette = m_ATITexturePalette;
        tmap.bClampS = FALSE;
        tmap.bClampT = FALSE;
        tmap.bAlphaBlend = FALSE;
        if (ATI3DCIF_TextureReg(&tmap, &m_ATITextureMap[i])) {
            LOG_ERROR("ERROR: Could not register texture");
            m_TextureLoaded[i] = false;
        } else {
            LOG_INFO(
                "    Texture %d, uploaded at %x", i, surface_desc.lpSurface);
            m_TextureLoaded[i] = true;
        }
    }

    m_SelectedTexture = -1;

    LOG_INFO("    complete");
}
