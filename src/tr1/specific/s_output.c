#include "specific/s_output.h"

#include "game/output.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "specific/s_shell.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/gfx/gl/track.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/log.h>

#include <string.h>

#define CLIP_VERTCOUNT_SCALE 4
#define MAP_DEPTH(zv) (g_FltResZBuf - g_FltResZ * (1.0 / (double)(zv)))
#define VBUF_VISIBLE(a, b, c)                                                  \
    (((a).ys - (b).ys) * ((c).xs - (b).xs)                                     \
     >= ((c).ys - (b).ys) * ((a).xs - (b).xs))

#define S_Output_CheckError(result)                                            \
    {                                                                          \
        if (!result) {                                                         \
            Shell_ExitSystem("Fatal 2D renderer error!");                      \
        }                                                                      \
    }

static int m_TextureMap[GFX_MAX_TEXTURES] = { GFX_NO_TEXTURE };
static int m_EnvMapTexture = GFX_NO_TEXTURE;

static GFX_2D_RENDERER *m_Renderer2D = nullptr;
static GFX_3D_RENDERER *m_Renderer3D = nullptr;
static bool m_IsTextureMode = false;
static int32_t m_SelectedTexture = -1;

static int32_t m_SurfaceWidth = 0;
static int32_t m_SurfaceHeight = 0;
static float m_SurfaceMinX = 0.0f;
static float m_SurfaceMinY = 0.0f;
static float m_SurfaceMaxX = 0.0f;
static float m_SurfaceMaxY = 0.0f;
static GFX_2D_SURFACE *m_PrimarySurface = nullptr;
static GFX_2D_SURFACE *m_PictureSurface = nullptr;
static GFX_2D_SURFACE *m_TextureSurfaces[GFX_MAX_TEXTURES] = { nullptr };

static inline float M_GetUV(uint16_t uv);
static void M_ReleaseTextures(void);
static void M_ReleaseSurfaces(void);
static void M_FlipPrimaryBuffer(void);
static void M_ClearSurface(GFX_2D_SURFACE *surface);
static void M_DrawTriangleFan(GFX_3D_VERTEX *vertices, int vertex_count);
static void M_DrawTriangleStrip(GFX_3D_VERTEX *vertices, int vertex_count);
static int32_t M_VisibleZClip(
    const PHD_VBUF *vn1, const PHD_VBUF *vn2, const PHD_VBUF *vn3);
static int32_t M_ZedClipper(
    int32_t vertex_count, const PHD_VBUF *vns[], GFX_3D_VERTEX *vertices);

static inline float M_GetUV(const uint16_t uv)
{
    return g_Config.rendering.pretty_pixels
            && g_Config.rendering.texture_filter == GFX_TF_NN
        ? uv / 65536.0f
        : ((uv & 0xFF00) + 127) / 65536.0f;
}

static void M_ReleaseTextures(void)
{
    if (m_Renderer3D == nullptr) {
        return;
    }
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (m_TextureMap[i] != GFX_NO_TEXTURE) {
            GFX_3D_Renderer_UnregisterTexturePage(
                m_Renderer3D, m_TextureMap[i]);
            m_TextureMap[i] = GFX_NO_TEXTURE;
        }
    }
    if (m_EnvMapTexture != GFX_NO_TEXTURE) {
        GFX_3D_Renderer_UnregisterEnvironmentMap(m_Renderer3D, m_EnvMapTexture);
    }
}

static void M_ReleaseSurfaces(void)
{
    if (m_PrimarySurface) {
        M_ClearSurface(m_PrimarySurface);

        GFX_2D_Surface_Free(m_PrimarySurface);
        m_PrimarySurface = nullptr;
    }

    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (m_TextureSurfaces[i] != nullptr) {
            GFX_2D_Surface_Free(m_TextureSurfaces[i]);
            m_TextureSurfaces[i] = nullptr;
        }
    }

    if (m_PictureSurface) {
        GFX_2D_Surface_Free(m_PictureSurface);
        m_PictureSurface = nullptr;
    }
}

void Output_FillEnvironmentMap(void)
{
    GFX_3D_Renderer_FillEnvironmentMap(m_Renderer3D);
}

static void M_FlipPrimaryBuffer(void)
{
    GFX_Context_SwapBuffers();
}

static void M_ClearSurface(GFX_2D_SURFACE *surface)
{
    GFX_2D_Surface_Clear(surface, 0);
}

static void M_DrawTriangleFan(GFX_3D_VERTEX *vertices, int vertex_count)
{
    GFX_3D_Renderer_RenderPrimFan(m_Renderer3D, vertices, vertex_count);
}

static void M_DrawTriangleStrip(GFX_3D_VERTEX *vertices, int vertex_count)
{
    GFX_3D_Renderer_RenderPrimStrip(m_Renderer3D, vertices, vertex_count);
}

static int32_t M_VisibleZClip(
    const PHD_VBUF *const vn1, const PHD_VBUF *const vn2,
    const PHD_VBUF *const vn3)
{
    double v1x = vn1->xv;
    double v1y = vn1->yv;
    double v1z = vn1->zv;
    double v2x = vn2->xv;
    double v2y = vn2->yv;
    double v2z = vn2->zv;
    double v3x = vn3->xv;
    double v3y = vn3->yv;
    double v3z = vn3->zv;
    double a = v3y * v1x - v1y * v3x;
    double b = v3x * v1z - v1x * v3z;
    double c = v3z * v1y - v1z * v3y;
    return a * v2z + b * v2y + c * v2x < 0.0;
}

static int32_t M_ZedClipper(
    const int32_t vertex_count, const PHD_VBUF *vns[],
    GFX_3D_VERTEX *const vertices)
{
    const float multiplier = g_Config.visuals.brightness / 16.0f;
    const float near_z = Output_GetNearZ();
    const float persp_o_near_z = (double)g_PhdPersp / near_z;

    GFX_3D_VERTEX *v = &vertices[0];
    int32_t current = 0;
    int32_t prev = vertex_count - 1;
    for (int32_t i = 0; i < vertex_count; i++) {
        const PHD_VBUF *vn0 = vns[current];
        const PHD_VBUF *vn1 = vns[prev];
        const int32_t diff0 = near_z - vn0->zv;
        const int32_t diff1 = near_z - vn1->zv;
        if ((diff0 | diff1) >= 0) {
            goto loop_end;
        }

        if ((diff0 ^ diff1) < 0) {
            const double clip = diff0 / (vn1->zv - vn0->zv);
            v->x = (vn0->xv + (vn1->xv - vn0->xv) * clip) * persp_o_near_z
                + Viewport_GetCenterX();
            v->y = (vn0->yv + (vn1->yv - vn0->yv) * clip) * persp_o_near_z
                + Viewport_GetCenterY();
            v->z = MAP_DEPTH(vn0->zv + (vn1->zv - vn0->zv) * clip);

            v->w = 1.0f / near_z;
            v->s = M_GetUV(vn0->u) + (M_GetUV(vn1->u) - M_GetUV(vn0->u)) * clip;
            v->t = M_GetUV(vn0->v) + (M_GetUV(vn1->v) - M_GetUV(vn0->v)) * clip;
            v->tex_coord[2] = vn0->tex_coord[2]
                + (vn1->tex_coord[2] - vn0->tex_coord[2]) * clip;
            v->tex_coord[3] = vn0->tex_coord[3]
                + (vn1->tex_coord[3] - vn0->tex_coord[3]) * clip;

            v->r = v->g = v->b =
                (8192.0f - (vn0->g + (vn1->g - vn0->g) * clip)) * multiplier;
            Output_ApplyTint(&v->r, &v->g, &v->b);

            v++;
        }

        if (diff0 < 0) {
            v->x = vn0->xs;
            v->y = vn0->ys;
            v->z = MAP_DEPTH(vn0->zv);

            v->w = 1.0f / vn0->zv;
            v->s = M_GetUV(vn0->u);
            v->t = M_GetUV(vn0->v);
            v->tex_coord[2] = vn0->tex_coord[2];
            v->tex_coord[3] = vn0->tex_coord[3];

            v->r = v->g = v->b = (8192.0f - vn0->g) * multiplier;
            Output_ApplyTint(&v->r, &v->g, &v->b);

            v++;
        }

    loop_end:
        prev = current++;
    }

    const int32_t count = v - vertices;
    return count < 3 ? 0 : count;
}

void S_Output_EnableTextureMode(void)
{
    if (m_IsTextureMode) {
        return;
    }

    m_IsTextureMode = true;
    GFX_3D_Renderer_SetTexturingEnabled(m_Renderer3D, m_IsTextureMode);
}

void S_Output_DisableTextureMode(void)
{
    if (!m_IsTextureMode) {
        return;
    }

    m_IsTextureMode = false;
    GFX_3D_Renderer_SetTexturingEnabled(m_Renderer3D, m_IsTextureMode);
}

void S_Output_SetBlendingMode(const GFX_BLEND_MODE blend_mode)
{
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, blend_mode);
}

void S_Output_EnableDepthWrites(void)
{
    GFX_3D_Renderer_SetDepthWritesEnabled(m_Renderer3D, true);
}

void S_Output_DisableDepthWrites(void)
{
    GFX_3D_Renderer_SetDepthWritesEnabled(m_Renderer3D, false);
}

void S_Output_EnableDepthTest(void)
{
    GFX_3D_Renderer_SetDepthTestEnabled(m_Renderer3D, true);
}

void S_Output_DisableDepthTest(void)
{
    GFX_3D_Renderer_SetDepthTestEnabled(m_Renderer3D, false);
}

void S_Output_RenderBegin(void)
{
    GFX_Context_Clear();
    GFX_Track_Reset();
    S_Output_DrawBackdropSurface();
    GFX_3D_Renderer_RenderBegin(m_Renderer3D);
    GFX_3D_Renderer_SetTextureFilter(
        m_Renderer3D, g_Config.rendering.texture_filter);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
}

void S_Output_RenderEnd(void)
{
    GFX_3D_Renderer_RenderEnd(m_Renderer3D);
}

void S_Output_Flush(void)
{
    GFX_3D_Renderer_Flush(m_Renderer3D);
}

void S_Output_FlipScreen(void)
{
    M_FlipPrimaryBuffer();
    m_SelectedTexture = -1;
}

void S_Output_ClearDepthBuffer(void)
{
    GFX_3D_Renderer_ClearDepth(m_Renderer3D);
}

void S_Output_DrawBackdropSurface(void)
{
    if (m_PictureSurface == nullptr) {
        return;
    }
    GFX_2D_Renderer_Render(m_Renderer2D);
}

void S_Output_DownloadBackdropSurface(const IMAGE *const image)
{
    GFX_2D_Surface_Free(m_PictureSurface);
    m_PictureSurface = nullptr;

    if (image == nullptr) {
        return;
    }

    m_PictureSurface = GFX_2D_Surface_CreateFromImage(image);
    GFX_2D_Renderer_Upload(
        m_Renderer2D, &m_PictureSurface->desc, m_PictureSurface->buffer);
}

void S_Output_SelectTexture(const int32_t texture_num)
{
    if (texture_num == m_SelectedTexture) {
        return;
    }

    if (m_TextureMap[texture_num] == GFX_NO_TEXTURE) {
        LOG_ERROR("ERROR: Attempt to select unloaded texture");
        return;
    }

    GFX_3D_Renderer_SelectTexture(m_Renderer3D, m_TextureMap[texture_num]);

    m_SelectedTexture = texture_num;
}

void S_Output_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int y2, int z, int sprnum, int shade)
{
    int vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count * CLIP_VERTCOUNT_SCALE];

    float multiplier = g_Config.visuals.brightness / 16.0f;

    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprnum);
    float vshade = (8192.0f - shade) * multiplier;
    if (vshade >= 256.0f) {
        vshade = 255.0f;
    }

    const float u0 = (sprite->offset & 0xFF) / 256.0f;
    const float v0 = (sprite->offset >> 8) / 256.0f;
    const float u1 = (sprite->width >> 8) / 256.0f + u0;
    const float v1 = (sprite->height >> 8) / 256.0f + v0;
    const float vz = MAP_DEPTH(z);
    const float rhw = 1.0f / z;

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = vz;
    vertices[0].s = u0;
    vertices[0].t = v0;
    vertices[0].tex_coord[2] = 1.0;
    vertices[0].tex_coord[3] = 1.0;
    vertices[0].w = rhw;
    vertices[0].r = vshade;
    vertices[0].g = vshade;
    vertices[0].b = vshade;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = vz;
    vertices[1].s = u1;
    vertices[1].t = v0;
    vertices[1].tex_coord[2] = 1.0;
    vertices[1].tex_coord[3] = 1.0;
    vertices[1].w = rhw;
    vertices[1].r = vshade;
    vertices[1].g = vshade;
    vertices[1].b = vshade;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = vz;
    vertices[2].s = u1;
    vertices[2].t = v1;
    vertices[2].tex_coord[2] = 1.0;
    vertices[2].tex_coord[3] = 1.0;
    vertices[2].w = rhw;
    vertices[2].r = vshade;
    vertices[2].g = vshade;
    vertices[2].b = vshade;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = vz;
    vertices[3].s = u0;
    vertices[3].t = v1;
    vertices[3].tex_coord[2] = 1.0;
    vertices[3].tex_coord[3] = 1.0;
    vertices[3].w = rhw;
    vertices[3].r = vshade;
    vertices[3].g = vshade;
    vertices[3].b = vshade;

    if (!vertex_count) {
        return;
    }

    if (m_TextureMap[sprite->tex_page] != GFX_NO_TEXTURE) {
        S_Output_EnableTextureMode();
        S_Output_SelectTexture(sprite->tex_page);
        M_DrawTriangleFan(vertices, vertex_count);
    } else {
        S_Output_DisableTextureMode();
        M_DrawTriangleFan(vertices, vertex_count);
    }
}

void S_Output_Draw3DLine(
    const PHD_VBUF *const vn0, const PHD_VBUF *const vn1, const RGBA_8888 color)
{
    int32_t vertex_count = 2;
    GFX_3D_VERTEX vertices[vertex_count * CLIP_VERTCOUNT_SCALE];
    const PHD_VBUF *const src_vbuf[2] = { vn0, vn1 };

    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].x = src_vbuf[i]->xs;
        vertices[i].y = src_vbuf[i]->ys;
        vertices[i].z = MAP_DEPTH(src_vbuf[i]->zv);
        vertices[i].r = color.r;
        vertices[i].g = color.g;
        vertices[i].b = color.b;
        vertices[i].a = color.a;
    }

    if ((vn0->clip | vn1->clip) < 0) {
        vertex_count =
            M_ZedClipper(vertex_count, (const PHD_VBUF **)src_vbuf, vertices);
        if (vertex_count == 0) {
            return;
        }
        for (int32_t i = 0; i < vertex_count; i++) {
            vertices[i].r = color.r;
            vertices[i].g = color.g;
            vertices[i].b = color.b;
            vertices[i].a = color.a;
        }
    }

    if (!vertex_count) {
        return;
    }

    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_LINE);
    S_Output_DisableTextureMode();
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    GFX_3D_Renderer_RenderPrimList(m_Renderer3D, vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_TRI);
}

void S_Output_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br)
{
    int vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count];

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = 1.0f;
    vertices[0].r = tl.r;
    vertices[0].g = tl.g;
    vertices[0].b = tl.b;
    vertices[0].a = tl.a;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = 1.0f;
    vertices[1].r = tr.r;
    vertices[1].g = tr.g;
    vertices[1].b = tr.b;
    vertices[1].a = tr.a;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = 1.0f;
    vertices[2].r = br.r;
    vertices[2].g = br.g;
    vertices[2].b = br.b;
    vertices[2].a = br.a;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = 1.0f;
    vertices[3].r = bl.r;
    vertices[3].g = bl.g;
    vertices[3].b = bl.b;
    vertices[3].a = bl.a;

    S_Output_DisableTextureMode();
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    M_DrawTriangleFan(vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
}

void S_Output_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2)
{
    int vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count * CLIP_VERTCOUNT_SCALE];

    S_Output_DisableTextureMode();

    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = MAP_DEPTH(z1);
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
    vertices[2].z = MAP_DEPTH(z2);
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

    M_DrawTriangleFan(vertices, vertex_count);

    vertex_count = 4;
    vertices[0].x = thickness1 / 2 + x1;
    vertices[0].y = y1;
    vertices[0].z = MAP_DEPTH(z1);
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
    vertices[2].z = MAP_DEPTH(z2);
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

    M_DrawTriangleFan(vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
}

void S_Output_DrawShadow(PHD_VBUF *vbufs, int clip, int vertex_count)
{
    // needs to be more than 8 cause clipping might return more polygons.
    GFX_3D_VERTEX vertices[vertex_count * CLIP_VERTCOUNT_SCALE];

    for (int32_t i = 0; i < vertex_count; i++) {
        GFX_3D_VERTEX *vertex = &vertices[i];
        PHD_VBUF *vbuf = &vbufs[i];
        vertex->x = vbuf->xs;
        vertex->y = vbuf->ys;
        vertex->z = MAP_DEPTH(vbuf->zv - (12 << W2V_SHIFT));
        vertex->b = 0.0f;
        vertex->g = 0.0f;
        vertex->r = 0.0f;
        vertex->a = 128.0f;
    }

    if (vertex_count == 0) {
        return;
    }

    S_Output_DisableTextureMode();

    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    M_DrawTriangleFan(vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
}

void S_Output_ApplyRenderSettings(void)
{
    if (m_Renderer3D == nullptr) {
        return;
    }

    if (m_PictureSurface != nullptr
        && (Screen_GetResWidth() != m_SurfaceWidth
            || Screen_GetResHeight() != m_SurfaceHeight)) {
        GFX_2D_Surface_Free(m_PictureSurface);
        m_PictureSurface = nullptr;
    }

    m_SurfaceWidth = Screen_GetResWidth();
    m_SurfaceHeight = Screen_GetResHeight();
    m_SurfaceMinX = 0.0f;
    m_SurfaceMinY = 0.0f;
    m_SurfaceMaxX = Screen_GetResWidth() - 1.0f;
    m_SurfaceMaxY = Screen_GetResHeight() - 1.0f;

    GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
    GFX_Context_SetDisplayFilter(g_Config.rendering.fbo_filter);
    GFX_Context_SetDisplaySize(m_SurfaceWidth, m_SurfaceHeight);
    GFX_Context_SetRenderingMode(g_Config.rendering.render_mode);
    GFX_Context_SetWireframeMode(g_Config.rendering.enable_wireframe);
    GFX_Context_SetLineWidth(g_Config.rendering.wireframe_width);
    GFX_3D_Renderer_SetAnisotropyFilter(
        m_Renderer3D, g_Config.rendering.anisotropy_filter);

    if (m_PrimarySurface == nullptr) {
        GFX_2D_SURFACE_DESC surface_desc = {};
        m_PrimarySurface = GFX_2D_Surface_Create(&surface_desc);
    }
    M_ClearSurface(m_PrimarySurface);
}

void S_Output_SetWindowSize(int width, int height)
{
    GFX_Context_SetWindowSize(width, height);
}

bool S_Output_Init(void)
{
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        m_TextureMap[i] = GFX_NO_TEXTURE;
        m_TextureSurfaces[i] = nullptr;
    }

    m_Renderer2D = GFX_2D_Renderer_Create();
    m_Renderer3D = GFX_3D_Renderer_Create();

    S_Output_ApplyRenderSettings();
    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_TRI);
    GFX_3D_Renderer_SetAlphaThreshold(m_Renderer3D, 0.0);
    GFX_3D_Renderer_SetAlphaPointDiscard(m_Renderer3D, true);

    return true;
}

void S_Output_Shutdown(void)
{
    M_ReleaseTextures();
    M_ReleaseSurfaces();

    if (m_Renderer2D != nullptr) {
        GFX_2D_Renderer_Destroy(m_Renderer2D);
        m_Renderer2D = nullptr;
    }
    if (m_Renderer3D != nullptr) {
        GFX_3D_Renderer_Destroy(m_Renderer3D);
        m_Renderer3D = nullptr;
    }
    GFX_Context_Detach();
}

void S_Output_DrawFlatTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, RGBA_8888 color)
{
    int vertex_count = 3;
    GFX_3D_VERTEX vertices[vertex_count * CLIP_VERTCOUNT_SCALE];
    PHD_VBUF *src_vbuf[3];

    src_vbuf[0] = vn1;
    src_vbuf[1] = vn2;
    src_vbuf[2] = vn3;

    if (vn3->clip & vn2->clip & vn1->clip) {
        return;
    }

    float multiplier = g_Config.visuals.brightness / (16.0f * 255.0f);
    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].x = src_vbuf[i]->xs;
        vertices[i].y = src_vbuf[i]->ys;
        vertices[i].z = MAP_DEPTH(src_vbuf[i]->zv);
        const float light = (8192.0f - src_vbuf[i]->g) * multiplier;
        vertices[i].r = color.r * light;
        vertices[i].g = color.g * light;
        vertices[i].b = color.b * light;
        vertices[i].a = color.a;

        Output_ApplyTint(&vertices[i].r, &vertices[i].g, &vertices[i].b);
    }

    if ((vn1->clip | vn2->clip | vn3->clip) >= 0) {
        if (!VBUF_VISIBLE(*vn1, *vn2, *vn3)) {
            return;
        }
    } else {
        if (!M_VisibleZClip(vn1, vn2, vn3)) {
            return;
        }

        vertex_count =
            M_ZedClipper(vertex_count, (const PHD_VBUF **)src_vbuf, vertices);
        if (vertex_count == 0) {
            return;
        }
        for (int32_t i = 0; i < vertex_count; i++) {
            vertices[i].r *= color.r / 255.0f;
            vertices[i].g *= color.g / 255.0f;
            vertices[i].b *= color.b / 255.0f;
            vertices[i].a = color.a;
        }
    }

    if (!vertex_count) {
        return;
    }

    M_DrawTriangleFan(vertices, vertex_count);
}

void S_Output_DrawEnvMapTriangle(
    const PHD_VBUF *const vn1, const PHD_VBUF *const vn2,
    const PHD_VBUF *const vn3)
{
    int vertex_count = 3;
    GFX_3D_VERTEX vertices[vertex_count * CLIP_VERTCOUNT_SCALE];

    const float multiplier = g_Config.visuals.brightness / 16.0f;
    const PHD_VBUF *const src_vbuf[3] = { vn1, vn2, vn3 };

    if (vn3->clip & vn2->clip & vn1->clip) {
        return;
    }

    if (vn1->clip >= 0 && vn2->clip >= 0 && vn3->clip >= 0) {
        if (!VBUF_VISIBLE(*vn1, *vn2, *vn3)) {
            return;
        }

        for (int32_t i = 0; i < vertex_count; i++) {
            vertices[i].x = src_vbuf[i]->xs;
            vertices[i].y = src_vbuf[i]->ys;
            vertices[i].z = MAP_DEPTH(src_vbuf[i]->zv);

            vertices[i].w = 1.0f / src_vbuf[i]->zv;
            vertices[i].s = M_GetUV(src_vbuf[i]->u);
            vertices[i].t = M_GetUV(src_vbuf[i]->v);
            vertices[i].tex_coord[2] = src_vbuf[i]->tex_coord[2];
            vertices[i].tex_coord[3] = src_vbuf[i]->tex_coord[3];

            vertices[i].r = vertices[i].g = vertices[i].b =
                (8192.0f - src_vbuf[i]->g) * multiplier;

            Output_ApplyTint(&vertices[i].r, &vertices[i].g, &vertices[i].b);
        }
    } else {
        if (!M_VisibleZClip(vn1, vn2, vn3)) {
            return;
        }

        vertex_count =
            M_ZedClipper(vertex_count, (const PHD_VBUF **)src_vbuf, vertices);
        if (vertex_count == 0) {
            return;
        }
    }

    if (!vertex_count) {
        return;
    }

    S_Output_EnableTextureMode();
    GFX_3D_Renderer_SelectTexture(m_Renderer3D, m_EnvMapTexture);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_MULTIPLY);
    M_DrawTriangleFan(vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
    m_SelectedTexture = -1;
}

void S_Output_DrawEnvMapQuad(
    const PHD_VBUF *const vn1, const PHD_VBUF *const vn2,
    const PHD_VBUF *const vn3, const PHD_VBUF *const vn4)
{
    int vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count];

    if (vn4->clip | vn3->clip | vn2->clip | vn1->clip) {
        if ((vn4->clip & vn3->clip & vn2->clip & vn1->clip)) {
            return;
        }

        if (vn1->clip >= 0 && vn2->clip >= 0 && vn3->clip >= 0
            && vn4->clip >= 0) {
            if (!VBUF_VISIBLE(*vn1, *vn2, *vn3)) {
                return;
            }
        } else if (!M_VisibleZClip(vn1, vn2, vn3)) {
            return;
        }

        S_Output_DrawEnvMapTriangle(vn1, vn2, vn3);
        S_Output_DrawEnvMapTriangle(vn3, vn4, vn1);
        return;
    }

    if (!VBUF_VISIBLE(*vn1, *vn2, *vn3)) {
        return;
    }

    float multiplier = g_Config.visuals.brightness / 16.0f;

    const PHD_VBUF *const src_vbuf[4] = { vn2, vn1, vn3, vn4 };

    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].x = src_vbuf[i]->xs;
        vertices[i].y = src_vbuf[i]->ys;
        vertices[i].z = MAP_DEPTH(src_vbuf[i]->zv);

        vertices[i].w = 1.0f / src_vbuf[i]->zv;
        vertices[i].s = M_GetUV(src_vbuf[i]->u);
        vertices[i].t = M_GetUV(src_vbuf[i]->v);
        vertices[i].tex_coord[2] = src_vbuf[i]->tex_coord[2];
        vertices[i].tex_coord[3] = src_vbuf[i]->tex_coord[3];

        vertices[i].r = vertices[i].g = vertices[i].b =
            (8192.0f - src_vbuf[i]->g) * multiplier;

        Output_ApplyTint(&vertices[i].r, &vertices[i].g, &vertices[i].b);
    }

    S_Output_EnableTextureMode();
    GFX_3D_Renderer_SelectTexture(m_Renderer3D, m_EnvMapTexture);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_MULTIPLY);
    GFX_3D_Renderer_RenderPrimStrip(m_Renderer3D, vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
    m_SelectedTexture = -1;
}

void S_Output_DrawTexturedTriangle(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, int16_t tpage,
    uint16_t textype)
{
    int vertex_count = 3;
    GFX_3D_VERTEX vertices[vertex_count * CLIP_VERTCOUNT_SCALE];
    PHD_VBUF *src_vbuf[3];

    float multiplier = g_Config.visuals.brightness / 16.0f;

    src_vbuf[0] = vn1;
    src_vbuf[1] = vn2;
    src_vbuf[2] = vn3;

    if (vn3->clip & vn2->clip & vn1->clip) {
        return;
    }

    if (src_vbuf[0]->clip >= 0 && src_vbuf[1]->clip >= 0
        && src_vbuf[2]->clip >= 0) {
        if (!VBUF_VISIBLE(*src_vbuf[0], *src_vbuf[1], *src_vbuf[2])) {
            return;
        }

        for (int32_t i = 0; i < vertex_count; i++) {
            vertices[i].x = src_vbuf[i]->xs;
            vertices[i].y = src_vbuf[i]->ys;
            vertices[i].z = MAP_DEPTH(src_vbuf[i]->zv);

            vertices[i].w = 1.0f / src_vbuf[i]->zv;
            vertices[i].s = M_GetUV(src_vbuf[i]->u);
            vertices[i].t = M_GetUV(src_vbuf[i]->v);
            vertices[i].tex_coord[2] = src_vbuf[i]->tex_coord[2];
            vertices[i].tex_coord[3] = src_vbuf[i]->tex_coord[3];

            vertices[i].r = vertices[i].g = vertices[i].b =
                (8192.0f - src_vbuf[i]->g) * multiplier;

            Output_ApplyTint(&vertices[i].r, &vertices[i].g, &vertices[i].b);
        }
    } else {
        if (!M_VisibleZClip(src_vbuf[0], src_vbuf[1], src_vbuf[2])) {
            return;
        }

        vertex_count =
            M_ZedClipper(vertex_count, (const PHD_VBUF **)src_vbuf, vertices);
        if (vertex_count == 0) {
            return;
        }
    }

    if (!vertex_count) {
        return;
    }

    if (m_TextureMap[tpage] != GFX_NO_TEXTURE) {
        S_Output_EnableTextureMode();
        S_Output_SelectTexture(tpage);
        M_DrawTriangleFan(vertices, vertex_count);
    } else {
        S_Output_DisableTextureMode();
        M_DrawTriangleFan(vertices, vertex_count);
    }
}

void S_Output_DrawTexturedQuad(
    PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3, PHD_VBUF *vn4, int16_t tpage,
    uint16_t textype)
{
    int vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count];
    PHD_VBUF *src_vbuf[4] = { vn1, vn2, vn3, vn4 };

    if (src_vbuf[3]->clip | src_vbuf[2]->clip | src_vbuf[1]->clip
        | src_vbuf[0]->clip) {
        if ((src_vbuf[3]->clip & src_vbuf[2]->clip & src_vbuf[1]->clip
             & src_vbuf[0]->clip)) {
            return;
        }

        if (src_vbuf[0]->clip >= 0 && src_vbuf[1]->clip >= 0
            && src_vbuf[2]->clip >= 0 && src_vbuf[3]->clip >= 0) {
            if (!VBUF_VISIBLE(*src_vbuf[0], *src_vbuf[1], *src_vbuf[2])) {
                return;
            }
        } else if (!M_VisibleZClip(src_vbuf[0], src_vbuf[1], src_vbuf[2])) {
            return;
        }

        S_Output_DrawTexturedTriangle(vn1, vn2, vn3, tpage, textype);
        S_Output_DrawTexturedTriangle(vn3, vn4, vn1, tpage, textype);
        return;
    }

    if (!VBUF_VISIBLE(*src_vbuf[0], *src_vbuf[1], *src_vbuf[2])) {
        return;
    }

    float multiplier = g_Config.visuals.brightness / 16.0f;

    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].x = src_vbuf[i]->xs;
        vertices[i].y = src_vbuf[i]->ys;
        vertices[i].z = MAP_DEPTH(src_vbuf[i]->zv);

        vertices[i].w = 1.0f / src_vbuf[i]->zv;
        vertices[i].s = M_GetUV(src_vbuf[i]->u);
        vertices[i].t = M_GetUV(src_vbuf[i]->v);
        vertices[i].tex_coord[2] = src_vbuf[i]->tex_coord[2];
        vertices[i].tex_coord[3] = src_vbuf[i]->tex_coord[3];

        vertices[i].r = vertices[i].g = vertices[i].b =
            (8192.0f - src_vbuf[i]->g) * multiplier;

        Output_ApplyTint(&vertices[i].r, &vertices[i].g, &vertices[i].b);
    }

    if (m_TextureMap[tpage] != GFX_NO_TEXTURE) {
        S_Output_EnableTextureMode();
        S_Output_SelectTexture(tpage);
    } else {
        S_Output_DisableTextureMode();
    }

    GFX_3D_Renderer_RenderPrimFan(m_Renderer3D, vertices, vertex_count);
}

void S_Output_DownloadTextures(int32_t pages)
{
    if (pages > GFX_MAX_TEXTURES) {
        Shell_ExitSystem("Attempt to download more than texture page limit");
    }

    M_ReleaseTextures();

    for (int32_t i = 0; i < pages; i++) {
        if (m_TextureSurfaces[i] == nullptr) {
            const GFX_2D_SURFACE_DESC surface_desc = {
                .width = TEXTURE_PAGE_WIDTH,
                .height = TEXTURE_PAGE_HEIGHT,
            };
            m_TextureSurfaces[i] = GFX_2D_Surface_Create(&surface_desc);
        }
        GFX_2D_SURFACE *const surface = m_TextureSurfaces[i];
        RGBA_8888 *const output_ptr = (RGBA_8888 *)surface->buffer;
        const RGBA_8888 *const input_ptr = Output_GetTexturePage32(i);
        memcpy(
            output_ptr, input_ptr,
            surface->desc.width * surface->desc.height * sizeof(RGBA_8888));

        m_TextureMap[i] = GFX_3D_Renderer_RegisterTexturePage(
            m_Renderer3D, output_ptr, surface->desc.width,
            surface->desc.height);
    }

    m_SelectedTexture = -1;

    m_EnvMapTexture = GFX_3D_Renderer_RegisterEnvironmentMap(m_Renderer3D);
}

void S_Output_DrawScreenFrame(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 col_dark, const RGBA_8888 col_light, const float thickness)
{
#define SB_NUM_VERTS_DARK 12
#define SB_NUM_VERTS_LIGHT 10
    GFX_3D_VERTEX vertices[SB_NUM_VERTS_DARK + SB_NUM_VERTS_LIGHT];
    GFX_3D_VERTEX *const dark_vertices = vertices;
    GFX_3D_VERTEX *const light_vertices = vertices + SB_NUM_VERTS_DARK;
    const float sxf = sx + thickness;
    const float syf = sy + thickness;
    const float hf = h;
    const float wf = w;

#define SET(i, x_, y_)                                                         \
    vertices[i].x = x_;                                                        \
    vertices[i].y = y_;
    // clang-format off
    // Top Left Dark edge
    SET(0,  sxf,                         syf + hf - thickness);
    SET(1,  sxf + thickness,             syf + hf - thickness);
    SET(2,  sxf,                         syf);
    SET(3,  sxf + thickness,             syf + thickness);
    SET(4,  sxf + wf - thickness,        syf);
    SET(5,  sxf + wf - thickness,        syf + thickness);
    // Bottom Right Dark set
    SET(6,  sxf + wf + thickness,        syf - thickness);
    SET(7,  sxf + wf,                    syf - thickness);
    SET(8,  sxf + wf + thickness,        syf + hf + thickness);
    SET(9,  sxf + wf,                    syf + hf);
    SET(10, sxf - thickness,             syf + hf + thickness);
    SET(11, sxf - thickness,             syf + hf);
    // Light box
    SET(12, sxf - thickness,             syf + hf);
    SET(13, sxf - thickness + thickness, syf + hf - thickness);
    SET(14, sxf - thickness,             syf - thickness);
    SET(15, sxf,                         syf);
    SET(16, sxf + wf,                    syf - thickness);
    SET(17, sxf + wf - thickness,        syf);
    SET(18, sxf + wf,                    syf + hf);
    SET(19, sxf + wf - thickness,        syf + hf - thickness);
    SET(20, sxf - thickness,             syf + hf);
    SET(21, sxf - thickness + thickness, syf + hf - thickness);
    // clang-format on
#undef SET

    for (int32_t i = 0; i < SB_NUM_VERTS_DARK + SB_NUM_VERTS_LIGHT; i++) {
        vertices[i].z = 1.0f;
        vertices[i].s = 0.0f;
        vertices[i].t = 0.0f;
        vertices[i].w = 0.0f;
    }
    for (int32_t i = 0; i < SB_NUM_VERTS_DARK; i++) {
        dark_vertices[i].r = col_dark.r;
        dark_vertices[i].g = col_dark.g;
        dark_vertices[i].b = col_dark.b;
        dark_vertices[i].a = col_dark.a;
    }
    for (int32_t i = 0; i < SB_NUM_VERTS_LIGHT; i++) {
        light_vertices[i].r = col_light.r;
        light_vertices[i].g = col_light.g;
        light_vertices[i].b = col_light.b;
        light_vertices[i].a = col_light.a;
    }
    S_Output_DisableTextureMode();
    M_DrawTriangleStrip(vertices, SB_NUM_VERTS_DARK + SB_NUM_VERTS_LIGHT);
}

void S_Output_4ColourTextBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br, float thickness)
{
    //  0                 2
    //   *               &
    //    1             3
    //
    //    7             5
    //   #               @
    //  6                 4
    GFX_3D_VERTEX vertices[10];

#define SET(i, x_, y_, color)                                                  \
    vertices[i].x = x_;                                                        \
    vertices[i].y = y_;                                                        \
    vertices[i].r = color.r;                                                   \
    vertices[i].g = color.g;                                                   \
    vertices[i].b = color.b;                                                   \
    vertices[i].a = color.a;
    // clang-format off
    SET(0, sx - thickness,     sy - thickness,     tl);
    SET(1, sx + thickness,     sy + thickness,     tl);
    SET(2, sx + w + thickness, sy - thickness,     tr);
    SET(3, sx + w - thickness, sy + thickness,     tr);
    SET(4, sx + w + thickness, sy + h + thickness, br);
    SET(5, sx + w - thickness, sy + h - thickness, br);
    SET(6, sx - thickness,     sy + h + thickness, bl);
    SET(7, sx + thickness,     sy + h - thickness, bl);
    SET(8, sx - thickness,     sy - thickness,     tl);
    SET(9, sx + thickness,     sy + thickness,     tl);
    // clang-format on
#undef SET

    for (int32_t i = 0; i < 10; i++) {
        vertices[i].z = 1.0f;
        vertices[i].s = 0.0f;
        vertices[i].t = 0.0f;
        vertices[i].w = 0.0f;
    }
    S_Output_DisableTextureMode();
    M_DrawTriangleStrip(vertices, 10);
}

void S_Output_2ToneColourTextBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 edge,
    RGBA_8888 centre, float thickness)
{
    //  0        2        4
    //   *               &
    //    1      3      5
    //
    // 14 15            7 6
    //
    //    13    10      9
    //   #               @
    // 12       11        8

    int32_t half_w = w / 2;
    int32_t half_h = h / 2;
    GFX_3D_VERTEX vertices[18];

#define SET(i, x_, y_, color)                                                  \
    vertices[i].x = x_;                                                        \
    vertices[i].y = y_;                                                        \
    vertices[i].r = color.r;                                                   \
    vertices[i].g = color.g;                                                   \
    vertices[i].b = color.b;                                                   \
    vertices[i].a = color.a;
    // clang-format off
    SET(0, sx - thickness,     sy - thickness,     edge);
    SET(1, sx + thickness,     sy + thickness,     edge);
    SET(2, sx + half_w,        sy - thickness,     centre);
    SET(3, sx + half_w,        sy + thickness,     centre);
    SET(4, sx + w + thickness, sy - thickness,     edge);
    SET(5, sx + w - thickness, sy + thickness,     edge);
    SET(6, sx + w + thickness, sy + half_h,        centre);
    SET(7, sx + w - thickness, sy + half_h,        centre);
    SET(8, sx + w + thickness, sy + h + thickness, edge);
    SET(9, sx + w - thickness, sy + h - thickness, edge);
    SET(10, sx + half_w,       sy + h + thickness, centre);
    SET(11, sx + half_w,       sy + h - thickness, centre);
    SET(12, sx - thickness,    sy + h + thickness, edge);
    SET(13, sx + thickness,    sy + h - thickness, edge);
    SET(14, sx - thickness,    sy + half_h,        centre);
    SET(15, sx + thickness,    sy + half_h,        centre);
    SET(16, sx - thickness,    sy - thickness,     edge);
    SET(17, sx + thickness,    sy + thickness,     edge);
    // clang-format on
#undef SET

    for (int32_t i = 0; i < 18; i++) {
        vertices[i].z = 1.0f;
        vertices[i].s = 0.0f;
        vertices[i].t = 0.0f;
        vertices[i].w = 0.0f;
    }
    S_Output_DisableTextureMode();
    M_DrawTriangleStrip(vertices, 18);
}

float Output_AdjustUV(const uint16_t uv)
{
    return M_GetUV(uv);
}
