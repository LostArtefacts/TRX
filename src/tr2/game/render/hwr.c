#include "game/render/hwr.h"

#include "decomp/decomp.h"
#include "game/output.h"
#include "game/render/priv.h"
#include "game/shell.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/gfx/3d/3d_renderer.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#define MAKE_DEPTH_FROM_RHW(rhw) (g_FltResZBuf - g_FltResZORhw * (rhw))
#define MAKE_DEPTH(v) MAKE_DEPTH_FROM_RHW((v)->rhw)

typedef enum {
    POLY_HWR_GTMAP,
    POLY_HWR_WGTMAP,
    POLY_HWR_GOURAUD,
    POLY_HWR_LINE,
    POLY_HWR_TRANS,
} POLY_HWR_TYPE;

typedef struct {
    GFX_2D_RENDERER *renderer_2d;
    GFX_3D_RENDERER *renderer_3d;
    GFX_2D_SURFACE *surface_pic;
    GFX_2D_SURFACE *surface_tex[GFX_MAX_TEXTURES];
    int32_t texture_map[GFX_MAX_TEXTURES];
    int32_t env_map_texture;
    int32_t current_texture;
} M_PRIV;

static VERTEX_INFO m_VBuffer[32] = {};
static GFX_3D_VERTEX m_VBufferGL[32] = {};
static GFX_3D_VERTEX m_HWR_VertexBuffer[MAX_VERTICES] = {};
static GFX_3D_VERTEX *m_HWR_VertexPtr = nullptr;

static void M_ShadeColor(
    GFX_3D_VERTEX *target, uint32_t red, uint32_t green, uint32_t blue,
    uint8_t alpha);
static void M_ShadeLight(
    GFX_3D_VERTEX *target, uint32_t shade, bool is_textured);
static void M_ShadeLightColor(
    GFX_3D_VERTEX *target, uint32_t shade, bool is_textured, uint32_t red,
    uint32_t green, uint32_t blue, uint8_t alpha);

static void M_ReleaseTextures(RENDERER *renderer);
static void M_LoadTexturePages(RENDERER *renderer);
static void M_SelectTexture(RENDERER *renderer, int32_t tex_source);
static void M_EnableColorKey(RENDERER *renderer, bool state);
static bool M_VertexBufferFull(void);

static void M_DrawPrimitive(
    RENDERER *renderer, GFX_3D_PRIM_TYPE primitive_type,
    const GFX_3D_VERTEX *vertices, int32_t vtx_count, bool is_no_clip);
static void M_DrawPolyTextured(RENDERER *renderer, int32_t vtx_count);
static void M_DrawPolyFlat(
    RENDERER *renderer, int32_t vtx_count, int32_t red, int32_t green,
    int32_t blue);

static void M_InsertPolyTextured(
    int32_t vtx_count, const float z, int16_t poly_type, int16_t tex_page);
static void M_InsertPolyFlat(
    int32_t vtx_count, const float z, int32_t red, int32_t green, int32_t blue,
    int16_t poly_type);

static void M_InsertGT3_Sorted(
    RENDERER *renderer, const PHD_VBUF *vtx0, const PHD_VBUF *vtx1,
    const PHD_VBUF *vtx2, const OBJECT_TEXTURE *texture, const TEXTURE_UV *uv0,
    const TEXTURE_UV *uv1, const TEXTURE_UV *uv2, const SORT_TYPE sort_type);
static void M_InsertGT4_Sorted(
    RENDERER *renderer, const PHD_VBUF *vtx0, const PHD_VBUF *vtx1,
    const PHD_VBUF *vtx2, const PHD_VBUF *vtx3, const OBJECT_TEXTURE *texture,
    const SORT_TYPE sort_type);
static void M_InsertFlatFace3s_Sorted(
    RENDERER *renderer, const FACE3 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertFlatFace4s_Sorted(
    RENDERER *renderer, const FACE4 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertTexturedFace3s_Sorted(
    RENDERER *renderer, const FACE3 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertTexturedFace4s_Sorted(
    RENDERER *renderer, const FACE4 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertFlatRect_Sorted(
    RENDERER *renderer, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
    int32_t z, uint8_t color_idx);
static void M_InsertLine_Sorted(
    RENDERER *renderer, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
    int32_t z, uint8_t color_idx);
static void M_InsertSprite_Sorted(
    RENDERER *renderer, int32_t z, int32_t x0, int32_t y0, int32_t x1,
    int32_t y1, int32_t sprite_idx, int16_t shade);
static void M_InsertTransQuad_Sorted(
    RENDERER *renderer, int32_t x, int32_t y, int32_t width, int32_t height,
    int32_t z);
static void M_InsertTransOctagon_Sorted(
    RENDERER *renderer, const PHD_VBUF *vbuf, int16_t shade);

static void M_InsertGT3_ZBuffered(
    RENDERER *renderer, const PHD_VBUF *vtx0, const PHD_VBUF *vtx1,
    const PHD_VBUF *vtx2, const OBJECT_TEXTURE *texture, const TEXTURE_UV *uv0,
    const TEXTURE_UV *uv1, const TEXTURE_UV *uv2);
static void M_InsertGT4_ZBuffered(
    RENDERER *renderer, const PHD_VBUF *vtx0, const PHD_VBUF *vtx1,
    const PHD_VBUF *vtx2, const PHD_VBUF *vtx3, const OBJECT_TEXTURE *texture);
static void M_InsertFlatFace3s_ZBuffered(
    RENDERER *renderer, const FACE3 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertFlatFace4s_ZBuffered(
    RENDERER *renderer, const FACE4 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertTexturedFace3s_ZBuffered(
    RENDERER *renderer, const FACE3 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertTexturedFace4s_ZBuffered(
    RENDERER *renderer, const FACE4 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertFlatRect_ZBuffered(
    RENDERER *renderer, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
    int32_t z, uint8_t color_idx);
static void M_InsertLine_ZBuffered(
    RENDERER *renderer, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
    int32_t z, uint8_t color_idx);

static void M_ResetParams(RENDERER *renderer);
static void M_ResetFuncPtrs(RENDERER *renderer);
static void M_Init(RENDERER *renderer);
static void M_Shutdown(RENDERER *renderer);
static void M_Open(RENDERER *renderer);
static void M_Close(RENDERER *renderer);
static void M_BeginScene(RENDERER *renderer);
static void M_EndScene(RENDERER *renderer);
static void M_Reset(RENDERER *renderer, RENDER_RESET_FLAGS flags);
static void M_ResetPolyList(RENDERER *renderer);
static void M_DrawPolyList(RENDERER *renderer);
static void M_EnableZBuffer(
    RENDERER *renderer, bool z_write_enable, bool z_test_enable);
static void M_ClearZBuffer(RENDERER *renderer);

static void M_ShadeColor(
    GFX_3D_VERTEX *const target, uint32_t red, uint32_t green,
    const uint32_t blue, const uint8_t alpha)
{
    if (Output_IsShadeEffect()) {
        red /= 2;
        green = green * 7 / 8;
    }
    target->r = red;
    target->g = green;
    target->b = blue;
    target->a = alpha;
}

static void M_ShadeLight(
    GFX_3D_VERTEX *const target, uint32_t shade, const bool is_textured)
{
    M_ShadeLightColor(target, shade, is_textured, 255, 255, 255, 255);
}

static void M_ShadeLightColor(
    GFX_3D_VERTEX *const target, uint32_t shade, const bool is_textured,
    uint32_t red, uint32_t green, uint32_t blue, const uint8_t alpha)
{
    CLAMPG(shade, 0x1FFF);

    if (g_Config.rendering.lighting_contrast == LIGHTING_CONTRAST_MEDIUM) {
        CLAMPL(shade, 0x800);
    }
    if (g_Config.rendering.lighting_contrast != LIGHTING_CONTRAST_LOW
        && is_textured) {
        shade = 0x1000 + shade / 2;
    }
    if (g_Config.rendering.lighting_contrast == LIGHTING_CONTRAST_LOW
        && !is_textured) {
        CLAMPL(shade, 0x1000);
    }

    if (shade != 0x1000) {
        const int32_t brightness = 0x1FFF - shade;
        red = (red * brightness) >> 12;
        green = (green * brightness) >> 12;
        blue = (blue * brightness) >> 12;
    }

    CLAMPG(red, 0xFF);
    CLAMPG(green, 0xFF);
    CLAMPG(blue, 0xFF);
    M_ShadeColor(target, red, green, blue, alpha);
}

static void M_ReleaseTextures(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    for (int32_t i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (priv->texture_map[i] != GFX_NO_TEXTURE) {
            GFX_3D_Renderer_UnregisterTexturePage(
                priv->renderer_3d, priv->texture_map[i]);
            priv->texture_map[i] = GFX_NO_TEXTURE;
        }
    }
    if (priv->env_map_texture != GFX_NO_TEXTURE) {
        GFX_3D_Renderer_UnregisterEnvironmentMap(
            priv->renderer_3d, priv->env_map_texture);
    }
}

static void M_LoadTexturePages(RENDERER *renderer)
{
    M_PRIV *const priv = renderer->priv;
    int32_t page_idx = -1;

    M_ReleaseTextures(renderer);

    const int32_t pages_count = Output_GetTexturePageCount();
    for (int32_t i = 0; i < pages_count; i++) {
        GFX_2D_SURFACE *const surface = priv->surface_tex[i];
        const RGBA_8888 *input_ptr = Output_GetTexturePage32(i);
        RGBA_8888 *output_ptr = (RGBA_8888 *)surface->buffer;
        memcpy(output_ptr, input_ptr, TEXTURE_PAGE_SIZE * sizeof(RGBA_8888));

        priv->texture_map[i] = GFX_3D_Renderer_RegisterTexturePage(
            priv->renderer_3d, surface->buffer, surface->desc.width,
            surface->desc.height);
    }

    priv->env_map_texture =
        GFX_3D_Renderer_RegisterEnvironmentMap(priv->renderer_3d);
}

static void M_SelectTexture(RENDERER *const renderer, const int32_t tex_source)
{
    M_PRIV *const priv = renderer->priv;
    if (!renderer->open) {
        return;
    }
    if (tex_source == -1) {
        priv->current_texture = tex_source;
        GFX_3D_Renderer_SetTexturingEnabled(priv->renderer_3d, false);
    } else if (tex_source != priv->current_texture) {
        priv->current_texture = tex_source;
        GFX_3D_Renderer_SetTexturingEnabled(priv->renderer_3d, true);
        GFX_3D_Renderer_SelectTexture(
            priv->renderer_3d, priv->texture_map[tex_source]);
    }
}

static void M_EnableColorKey(RENDERER *const renderer, const bool state)
{
    M_PRIV *const priv = renderer->priv;
    GFX_3D_Renderer_SetBlendingMode(
        priv->renderer_3d, state ? GFX_BLEND_MODE_NORMAL : GFX_BLEND_MODE_OFF);
}

static bool M_VertexBufferFull(void)
{
    const int32_t index = m_HWR_VertexPtr - m_HWR_VertexBuffer;
    return index >= MAX_VERTICES - 0x200;
}

static void M_DrawPrimitive(
    RENDERER *renderer, GFX_3D_PRIM_TYPE primitive_type,
    const GFX_3D_VERTEX *const vertices, int32_t vtx_count, bool is_no_clip)
{
    M_PRIV *const priv = renderer->priv;
    GFX_3D_Renderer_RenderPrimFan(priv->renderer_3d, vertices, vtx_count);
}

static void M_DrawPolyTextured(
    RENDERER *const renderer, const int32_t vtx_count)
{
    for (int32_t i = 0; i < vtx_count; i++) {
        const VERTEX_INFO *const vbuf = &m_VBuffer[i];
        GFX_3D_VERTEX *const vbuf_gl = &m_VBufferGL[i];
        vbuf_gl->x = vbuf->x;
        vbuf_gl->y = vbuf->y;
        vbuf_gl->z = MAKE_DEPTH(vbuf);
        vbuf_gl->w = vbuf->rhw;
        vbuf_gl->t = vbuf->v / (double)PHD_ONE;
        vbuf_gl->s = vbuf->u / (double)PHD_ONE;
        M_ShadeLight(vbuf_gl, vbuf->g, true);
    }
    M_DrawPrimitive(renderer, GFX_3D_PRIM_TRI, m_VBufferGL, vtx_count, true);
}

static void M_DrawPolyFlat(
    RENDERER *const renderer, const int32_t vtx_count, const int32_t red,
    const int32_t green, const int32_t blue)
{
    for (int32_t i = 0; i < vtx_count; i++) {
        const VERTEX_INFO *const vbuf = &m_VBuffer[i];
        GFX_3D_VERTEX *const vbuf_gl = &m_VBufferGL[i];
        vbuf_gl->x = vbuf->x;
        vbuf_gl->y = vbuf->y;
        vbuf_gl->z = MAKE_DEPTH(vbuf);
        vbuf_gl->w = vbuf->rhw;
        M_ShadeLightColor(vbuf_gl, vbuf->g, false, red, green, blue, 0xFF);
    }
    M_DrawPrimitive(renderer, GFX_3D_PRIM_TRI, m_VBufferGL, vtx_count, true);
}

static void M_InsertPolyTextured(
    const int32_t vtx_count, const float z, const int16_t poly_type,
    const int16_t tex_page)
{
    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = poly_type;
    *g_Info3DPtr++ = tex_page;
    *g_Info3DPtr++ = vtx_count;
    *(GFX_3D_VERTEX **)g_Info3DPtr = m_HWR_VertexPtr;
    g_Info3DPtr += sizeof(GFX_3D_VERTEX *) / sizeof(int16_t);

    for (int32_t i = 0; i < vtx_count; i++) {
        const VERTEX_INFO *const vbuf = &m_VBuffer[i];
        GFX_3D_VERTEX *const vbuf_gl = &m_HWR_VertexPtr[i];
        vbuf_gl->x = vbuf->x;
        vbuf_gl->y = vbuf->y;
        vbuf_gl->z = MAKE_DEPTH(vbuf);
        vbuf_gl->w = vbuf->rhw;
        vbuf_gl->s = vbuf->u / (double)PHD_ONE;
        vbuf_gl->t = vbuf->v / (double)PHD_ONE;
        M_ShadeLight(vbuf_gl, vbuf->g, true);
    }

    m_HWR_VertexPtr += vtx_count;
    g_SurfaceCount++;
}

static void M_InsertPolyFlat(
    const int32_t vtx_count, const float z, const int32_t red,
    const int32_t green, const int32_t blue, const int16_t poly_type)
{
    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = poly_type;
    *g_Info3DPtr++ = vtx_count;
    *(GFX_3D_VERTEX **)g_Info3DPtr = m_HWR_VertexPtr;
    g_Info3DPtr += sizeof(GFX_3D_VERTEX *) / sizeof(int16_t);

    for (int32_t i = 0; i < vtx_count; i++) {
        const VERTEX_INFO *const vbuf = &m_VBuffer[i];
        GFX_3D_VERTEX *const vbuf_gl = &m_HWR_VertexPtr[i];
        vbuf_gl->x = vbuf->x;
        vbuf_gl->y = vbuf->y;
        vbuf_gl->z = MAKE_DEPTH(vbuf);
        vbuf_gl->w = vbuf->rhw;
        M_ShadeLightColor(
            vbuf_gl, vbuf->g, false, red, green, blue,
            poly_type == POLY_HWR_TRANS ? 0x80 : 0xFF);
    }

    m_HWR_VertexPtr += vtx_count;
    g_SurfaceCount++;
}

static void M_InsertGT3_Sorted(
    RENDERER *const renderer, const PHD_VBUF *const vtx0,
    const PHD_VBUF *const vtx1, const PHD_VBUF *const vtx2,
    const OBJECT_TEXTURE *const texture, const TEXTURE_UV *const uv0,
    const TEXTURE_UV *const uv1, const TEXTURE_UV *const uv2,
    const SORT_TYPE sort_type)
{
    const int8_t clip_or = vtx0->clip | vtx1->clip | vtx2->clip;
    const int8_t clip_and = vtx0->clip & vtx1->clip & vtx2->clip;
    if (clip_and != 0) {
        return;
    }

    const double zv =
        Render_CalculatePolyZ(sort_type, vtx0->zv, vtx1->zv, vtx2->zv, -1.0);
    const POLY_HWR_TYPE poly_type =
        texture->draw_type == DRAW_OPAQUE ? POLY_HWR_GTMAP : POLY_HWR_WGTMAP;
    const PHD_VBUF *const vtx[3] = { vtx0, vtx1, vtx2 };
    const TEXTURE_UV *const uv[3] = { uv0, uv1, uv2 };

    int32_t num_points = 3;
    if (clip_or >= 0) {
        if (!VBUF_VISIBLE(*vtx0, *vtx1, *vtx2)) {
            return;
        }

        if (clip_or == 0) {
            g_Sort3DPtr->_0 = g_Info3DPtr;
            g_Sort3DPtr->_1 = MAKE_ZSORT(zv);
            g_Sort3DPtr++;

            *g_Info3DPtr++ = poly_type;
            *g_Info3DPtr++ = texture->tex_page;
            *g_Info3DPtr++ = num_points;
            *(GFX_3D_VERTEX **)g_Info3DPtr = m_HWR_VertexPtr;
            g_Info3DPtr += sizeof(GFX_3D_VERTEX *) / sizeof(int16_t);

            for (int32_t i = 0; i < 3; i++) {
                GFX_3D_VERTEX *const vbuf_gl = &m_HWR_VertexPtr[i];
                vbuf_gl->x = vtx[i]->xs;
                vbuf_gl->y = vtx[i]->ys;
                vbuf_gl->z = MAKE_DEPTH(vtx[i]);
                vbuf_gl->w = vtx[i]->rhw;
                vbuf_gl->s = (double)uv[i]->u * vtx[i]->rhw / (double)PHD_ONE;
                vbuf_gl->t = (double)uv[i]->v * vtx[i]->rhw / (double)PHD_ONE;
                M_ShadeLight(vbuf_gl, vtx[i]->g, true);
            }

            m_HWR_VertexPtr += 3;
            g_SurfaceCount++;
            return;
        }

        for (int32_t i = 0; i < 3; i++) {
            VERTEX_INFO *const vbuf = &m_VBuffer[i];
            vbuf->x = vtx[i]->xs;
            vbuf->y = vtx[i]->ys;
            vbuf->z = vtx[i]->zv;
            vbuf->rhw = vtx[i]->rhw;
            vbuf->g = (double)vtx[i]->g;
            vbuf->u = (double)uv[i]->u * vtx[i]->rhw;
            vbuf->v = (double)uv[i]->v * vtx[i]->rhw;
        }
    } else {
        if (!Render_VisibleZClip(vtx0, vtx1, vtx2)) {
            return;
        }

        POINT_INFO points[3];
        for (int32_t i = 0; i < 3; i++) {
            points[i].xv = vtx[i]->xv;
            points[i].yv = vtx[i]->yv;
            points[i].zv = vtx[i]->zv;
            points[i].rhw = vtx[i]->rhw;
            points[i].xs = vtx[i]->xs;
            points[i].ys = vtx[i]->ys;
            points[i].g = vtx[i]->g;
            points[i].u = uv[i]->u;
            points[i].v = uv[i]->v;
        }
        num_points = Render_ZedClipper(num_points, points, m_VBuffer);
        if (num_points == 0) {
            return;
        }
    }

    M_InsertPolyTextured(num_points, zv, poly_type, texture->tex_page);
}

static void M_InsertGT4_Sorted(
    RENDERER *const renderer, const PHD_VBUF *const vtx0,
    const PHD_VBUF *const vtx1, const PHD_VBUF *const vtx2,
    const PHD_VBUF *const vtx3, const OBJECT_TEXTURE *const texture,
    const SORT_TYPE sort_type)
{
    const int8_t clip_or = vtx0->clip | vtx1->clip | vtx2->clip | vtx3->clip;
    const int8_t clip_and = vtx0->clip & vtx1->clip & vtx2->clip & vtx3->clip;
    if (clip_and != 0) {
        return;
    }

    const double zv = Render_CalculatePolyZ(
        sort_type, vtx0->zv, vtx1->zv, vtx2->zv, vtx3->zv);
    const POLY_HWR_TYPE poly_type =
        texture->draw_type == DRAW_OPAQUE ? POLY_HWR_GTMAP : POLY_HWR_WGTMAP;
    const PHD_VBUF *const vtx[4] = { vtx0, vtx1, vtx2, vtx3 };

    int32_t num_points = 4;
    if (clip_or >= 0) {
        if (!VBUF_VISIBLE(*vtx0, *vtx1, *vtx2)) {
            return;
        }

        if (clip_or == 0) {
            g_Sort3DPtr->_0 = g_Info3DPtr;
            g_Sort3DPtr->_1 = MAKE_ZSORT(zv);
            g_Sort3DPtr++;

            *g_Info3DPtr++ = poly_type;
            *g_Info3DPtr++ = texture->tex_page;
            *g_Info3DPtr++ = num_points;
            *(GFX_3D_VERTEX **)g_Info3DPtr = m_HWR_VertexPtr;
            g_Info3DPtr += sizeof(GFX_3D_VERTEX *) / sizeof(int16_t);

            for (int32_t i = 0; i < 4; i++) {
                GFX_3D_VERTEX *const vbuf_gl = &m_HWR_VertexPtr[i];
                vbuf_gl->x = vtx[i]->xs;
                vbuf_gl->y = vtx[i]->ys;
                vbuf_gl->z = MAKE_DEPTH(vtx[i]);
                vbuf_gl->w = vtx[i]->rhw;
                vbuf_gl->s = texture->uv[i].u * vtx[i]->rhw / (double)PHD_ONE;
                vbuf_gl->t = texture->uv[i].v * vtx[i]->rhw / (double)PHD_ONE;
                M_ShadeLight(vbuf_gl, vtx[i]->g, true);
            }

            m_HWR_VertexPtr += 4;
            g_SurfaceCount++;
            return;
        }

        M_InsertGT3_Sorted(
            renderer, vtx0, vtx1, vtx2, texture, &texture->uv[0],
            &texture->uv[1], &texture->uv[2], sort_type);
        M_InsertGT3_Sorted(
            renderer, vtx0, vtx2, vtx3, texture, &texture->uv[0],
            &texture->uv[2], &texture->uv[3], sort_type);
    } else {
        if (!Render_VisibleZClip(vtx0, vtx1, vtx2)) {
            return;
        }

        M_InsertGT3_Sorted(
            renderer, vtx0, vtx1, vtx2, texture, &texture->uv[0],
            &texture->uv[1], &texture->uv[2], sort_type);
        M_InsertGT3_Sorted(
            renderer, vtx0, vtx2, vtx3, texture, &texture->uv[0],
            &texture->uv[2], &texture->uv[3], sort_type);
    }
}

static void M_InsertFlatFace3s_Sorted(
    RENDERER *const renderer, const FACE3 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    for (int32_t i = 0; i < num; i++) {
        if (M_VertexBufferFull()) {
            break;
        }

        int32_t num_points = 3;
        const FACE3 *const face = &faces[i];
        const PHD_VBUF *vtx[3] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
        };

        const int8_t clip_or = vtx[0]->clip | vtx[1]->clip | vtx[2]->clip;
        const int8_t clip_and = vtx[0]->clip & vtx[1]->clip & vtx[2]->clip;
        if (clip_and != 0) {
            continue;
        }

        if (clip_or >= 0) {
            if (!VBUF_VISIBLE(*vtx[0], *vtx[1], *vtx[2])) {
                continue;
            }

            for (int32_t i = 0; i < 3; i++) {
                VERTEX_INFO *const vbuf = &m_VBuffer[i];
                vbuf->x = vtx[i]->xs;
                vbuf->y = vtx[i]->ys;
                vbuf->z = vtx[i]->zv;
                vbuf->rhw = vtx[i]->rhw;
                vbuf->g = vtx[i]->g;
            }
            if (clip_or > 0) {
                num_points = Render_XYGClipper(num_points, m_VBuffer);
            }
        } else {
            if (!Render_VisibleZClip(vtx[0], vtx[1], vtx[2])) {
                continue;
            }

            POINT_INFO points[3];
            for (int32_t i = 0; i < 3; i++) {
                points[i].xv = vtx[i]->xv;
                points[i].yv = vtx[i]->yv;
                points[i].zv = vtx[i]->zv;
                points[i].rhw = vtx[i]->rhw;
                points[i].xs = vtx[i]->xs;
                points[i].ys = vtx[i]->ys;
                points[i].g = vtx[i]->g;
            }
            num_points = Render_ZedClipper(num_points, points, m_VBuffer);
            if (num_points == 0) {
                continue;
            }

            num_points = Render_XYGClipper(num_points, m_VBuffer);
        }

        if (num_points == 0) {
            continue;
        }

        const RGB_888 color = Output_GetPaletteColor16(face->palette_idx >> 8);
        const double zv = Render_CalculatePolyZ(
            sort_type, vtx[0]->zv, vtx[1]->zv, vtx[2]->zv, -1.0);
        M_InsertPolyFlat(
            num_points, zv, color.r, color.g, color.b, POLY_HWR_GOURAUD);
    }
}

static void M_InsertFlatFace4s_Sorted(
    RENDERER *const renderer, const FACE4 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    for (int32_t i = 0; i < num; i++) {
        if (M_VertexBufferFull()) {
            break;
        }

        int32_t num_points = 4;
        const FACE4 *const face = &faces[i];
        const PHD_VBUF *const vtx[4] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
            &g_PhdVBuf[face->vertices[3]],
        };

        const int8_t clip_or =
            vtx[0]->clip | vtx[1]->clip | vtx[2]->clip | vtx[3]->clip;
        const int8_t clip_and =
            vtx[0]->clip & vtx[1]->clip & vtx[2]->clip & vtx[3]->clip;
        if (clip_and != 0) {
            continue;
        }

        if (clip_or >= 0) {
            if (!VBUF_VISIBLE(*vtx[0], *vtx[1], *vtx[2])) {
                continue;
            }

            for (int32_t i = 0; i < 4; i++) {
                VERTEX_INFO *const vbuf = &m_VBuffer[i];
                vbuf->x = vtx[i]->xs;
                vbuf->y = vtx[i]->ys;
                vbuf->z = vtx[i]->zv;
                vbuf->rhw = vtx[i]->rhw;
                vbuf->g = vtx[i]->g;
            }

            if (clip_or > 0) {
                num_points = Render_XYGClipper(num_points, m_VBuffer);
            }
        } else {
            if (!Render_VisibleZClip(vtx[0], vtx[1], vtx[2])) {
                continue;
            }

            POINT_INFO points[4];
            for (int32_t i = 0; i < 4; i++) {
                points[i].xv = vtx[i]->xv;
                points[i].yv = vtx[i]->yv;
                points[i].zv = vtx[i]->zv;
                points[i].rhw = vtx[i]->rhw;
                points[i].xs = vtx[i]->xs;
                points[i].ys = vtx[i]->ys;
                points[i].g = vtx[i]->g;
            }
            num_points = Render_ZedClipper(num_points, points, m_VBuffer);
            if (num_points == 0) {
                continue;
            }
            num_points = Render_XYGClipper(num_points, m_VBuffer);
        }

        if (num_points == 0) {
            continue;
        }

        const RGB_888 color = Output_GetPaletteColor16(face->palette_idx >> 8);
        const double zv = Render_CalculatePolyZ(
            sort_type, vtx[0]->zv, vtx[1]->zv, vtx[2]->zv, vtx[3]->zv);
        M_InsertPolyFlat(
            num_points, zv, color.r, color.g, color.b, POLY_HWR_GOURAUD);
    }
}

static void M_InsertTexturedFace3s_Sorted(
    RENDERER *const renderer, const FACE3 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    for (int32_t i = 0; i < num; i++) {
        if (M_VertexBufferFull()) {
            break;
        }

        const FACE3 *const face = &faces[i];
        const PHD_VBUF *const vtx[3] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
        };

        const OBJECT_TEXTURE *const texture =
            Output_GetObjectTexture(face->texture_idx);
        const TEXTURE_UV *const uv = texture->uv;

        if (texture->draw_type != DRAW_OPAQUE && g_DiscardTransparent) {
            continue;
        }

        M_InsertGT3_Sorted(
            renderer, vtx[0], vtx[1], vtx[2], texture, &uv[0], &uv[1], &uv[2],
            sort_type);
    }
}

static void M_InsertTexturedFace4s_Sorted(
    RENDERER *const renderer, const FACE4 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    for (int32_t i = 0; i < num; i++) {
        if (M_VertexBufferFull()) {
            break;
        }

        const FACE4 *const face = &faces[i];
        const PHD_VBUF *const vtx[4] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
            &g_PhdVBuf[face->vertices[3]],
        };

        const OBJECT_TEXTURE *const texture =
            Output_GetObjectTexture(face->texture_idx);
        if (texture->draw_type != DRAW_OPAQUE && g_DiscardTransparent) {
            continue;
        }

        M_InsertGT4_Sorted(
            renderer, vtx[0], vtx[1], vtx[2], vtx[3], texture, sort_type);
    }
}

static void M_InsertFlatRect_Sorted(
    RENDERER *const renderer, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
    const int32_t z, const uint8_t color_idx)
{
    if (x2 <= x1 || y2 <= y1) {
        return;
    }

    CLAMPL(x1, 0);
    CLAMPL(y1, 0);
    CLAMPG(x2, g_PhdWinWidth);
    CLAMPG(y2, g_PhdWinHeight);

    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = POLY_HWR_GOURAUD;
    *g_Info3DPtr++ = 4;
    *(GFX_3D_VERTEX **)g_Info3DPtr = m_HWR_VertexPtr;
    g_Info3DPtr += sizeof(GFX_3D_VERTEX *) / sizeof(int16_t);

    const RGB_888 color = Output_GetPaletteColor8(color_idx);
    const double rhw = g_RhwFactor / (double)z;
    const double sz = MAKE_DEPTH_FROM_RHW(rhw);

    m_HWR_VertexPtr[0].x = x1;
    m_HWR_VertexPtr[0].y = y1;
    m_HWR_VertexPtr[1].x = x2;
    m_HWR_VertexPtr[1].y = y1;
    m_HWR_VertexPtr[2].x = x2;
    m_HWR_VertexPtr[2].y = y2;
    m_HWR_VertexPtr[3].x = x1;
    m_HWR_VertexPtr[3].y = y2;

    for (int32_t i = 0; i < 4; i++) {
        GFX_3D_VERTEX *const vbuf_gl = &m_HWR_VertexPtr[i];
        vbuf_gl->z = sz;
        vbuf_gl->w = rhw;
        M_ShadeColor(vbuf_gl, color.r, color.g, color.b, 0xFF);
    }

    m_HWR_VertexPtr += 4;
    g_SurfaceCount++;
}

static void M_InsertLine_Sorted(
    RENDERER *const renderer, const int32_t x1, const int32_t y1,
    const int32_t x2, const int32_t y2, int32_t z, const uint8_t color_idx)
{
    const RGB_888 color = Output_GetPaletteColor8(color_idx);
    const double rhw = g_RhwFactor / (double)z;
    const double sz = MAKE_DEPTH_FROM_RHW(rhw);

    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = POLY_HWR_LINE;
    *g_Info3DPtr++ = 2;
    *(GFX_3D_VERTEX **)g_Info3DPtr = m_HWR_VertexPtr;
    g_Info3DPtr += sizeof(GFX_3D_VERTEX *) / sizeof(int16_t);

    m_HWR_VertexPtr[0].x = x1;
    m_HWR_VertexPtr[0].y = y1;
    m_HWR_VertexPtr[1].x = x2;
    m_HWR_VertexPtr[1].y = y2;

    for (int32_t i = 0; i < 2; i++) {
        GFX_3D_VERTEX *const vbuf_gl = &m_HWR_VertexPtr[i];
        vbuf_gl->z = sz;
        vbuf_gl->w = rhw;
        M_ShadeColor(vbuf_gl, color.r, color.g, color.b, 0xFF);
    }

    m_HWR_VertexPtr += 2;
    g_SurfaceCount++;
}

static void M_InsertSprite_Sorted(
    RENDERER *const renderer, int32_t z, int32_t x0, int32_t y0, int32_t x1,
    int32_t y1, const int32_t sprite_idx, const int16_t shade)
{
    if (M_VertexBufferFull() || x0 >= x1 || y0 >= y1 || x1 <= 0 || y1 <= 0
        || x0 >= g_PhdWinMaxX || y0 >= g_PhdWinMaxY || z >= g_PhdFarZ) {
        return;
    }

    x0 += 0;
    x1 += 0;
    y0 += 0;
    y1 += 0;

    CLAMPL(z, g_PhdNearZ);

    int32_t num_points = 4;

    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);
    const double rhw = g_RhwFactor / (double)z;
    const int32_t u_offset = (sprite->offset & 0xFF) * 256;
    const int32_t v_offset = (sprite->offset >> 8) * 256;

    const int32_t offset = Render_GetUVAdjustment();
    const double u0 = (double)(u_offset + offset) * rhw;
    const double v0 = (double)(v_offset + offset) * rhw;
    const double u1 = (double)(u_offset - offset + sprite->width) * rhw;
    const double v1 = (double)(v_offset - offset + sprite->height) * rhw;

    m_VBuffer[0].x = x0;
    m_VBuffer[0].y = y0;
    m_VBuffer[0].u = u0;
    m_VBuffer[0].v = v0;

    m_VBuffer[1].x = x1;
    m_VBuffer[1].y = y0;
    m_VBuffer[1].u = u1;
    m_VBuffer[1].v = v0;

    m_VBuffer[2].x = x1;
    m_VBuffer[2].y = y1;
    m_VBuffer[2].u = u1;
    m_VBuffer[2].v = v1;

    m_VBuffer[3].x = x0;
    m_VBuffer[3].y = y1;
    m_VBuffer[3].u = u0;
    m_VBuffer[3].v = v1;

    for (int32_t i = 0; i < 4; i++) {
        VERTEX_INFO *const vbuf = &m_VBuffer[i];
        vbuf->rhw = rhw;
        vbuf->z = z;
        vbuf->g = shade;
    }

    if (x0 < 0 || y0 < 0 || x1 > g_PhdWinWidth || y1 > g_PhdWinHeight) {
        g_FltWinLeft = 0.0f;
        g_FltWinTop = 0.0f;
        g_FltWinRight = g_PhdWinWidth;
        g_FltWinBottom = g_PhdWinHeight;
    }

    const bool old_shade = Output_IsShadeEffect();
    Output_SetShadeEffect(false);
    M_InsertPolyTextured(num_points, z, POLY_HWR_WGTMAP, sprite->tex_page);
    Output_SetShadeEffect(old_shade);
}

static void M_InsertTransQuad_Sorted(
    RENDERER *const renderer, const int32_t x, const int32_t y,
    const int32_t width, const int32_t height, const int32_t z)
{
    const double x0 = (double)x;
    const double y0 = (double)y;
    const double x1 = (double)(x + width);
    const double y1 = (double)(y + height);
    const double rhw = g_RhwFactor / (double)z;
    const double sz = MAKE_DEPTH_FROM_RHW(rhw);

    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = POLY_HWR_TRANS;
    *g_Info3DPtr++ = 4;
    *(GFX_3D_VERTEX **)g_Info3DPtr = m_HWR_VertexPtr;
    g_Info3DPtr += sizeof(GFX_3D_VERTEX *) / sizeof(int16_t);

    m_HWR_VertexPtr[0].x = x0;
    m_HWR_VertexPtr[0].y = y0;
    m_HWR_VertexPtr[1].x = x1;
    m_HWR_VertexPtr[1].y = y0;
    m_HWR_VertexPtr[2].x = x1;
    m_HWR_VertexPtr[2].y = y1;
    m_HWR_VertexPtr[3].x = x0;
    m_HWR_VertexPtr[3].y = y1;

    for (int32_t i = 0; i < 4; i++) {
        GFX_3D_VERTEX *const vbuf_gl = &m_HWR_VertexPtr[i];
        vbuf_gl->z = sz;
        vbuf_gl->w = rhw;
        vbuf_gl->r = 0;
        vbuf_gl->g = 0;
        vbuf_gl->b = 0;
        vbuf_gl->a = 0x80;
    }

    m_HWR_VertexPtr += 4;
    g_SurfaceCount++;
}

static void M_InsertTransOctagon_Sorted(
    RENDERER *const renderer, const PHD_VBUF *const vtx, const int16_t shade)
{
    int8_t clip_or = 0x00;
    uint8_t clip_and = 0xFF;
    int32_t num_vtx = 8;

    for (int32_t i = 0; i < num_vtx; i++) {
        clip_or |= vtx[i].clip;
        clip_and &= vtx[i].clip;
    }

    if (clip_or < 0 || clip_and != 0 || !VBUF_VISIBLE(vtx[0], vtx[1], vtx[2])) {
        return;
    }

    for (int32_t i = 0; i < num_vtx; i++) {
        VERTEX_INFO *const vbuf = &m_VBuffer[i];
        vbuf->x = vtx[i].xs;
        vbuf->y = vtx[i].ys;
        vbuf->z = vtx[i].zv;
        vbuf->rhw = g_RhwFactor / (double)(vtx[i].zv - 0x20000);
    }

    int32_t num_points = num_vtx;
    if (clip_or != 0) {
        g_FltWinLeft = 0.0f;
        g_FltWinTop = 0.0f;
        g_FltWinRight = g_PhdWinWidth;
        g_FltWinBottom = g_PhdWinHeight;
        num_points = Render_XYClipper(num_points, m_VBuffer);
        if (num_points == 0) {
            return;
        }
    }

    double poly_z = 0.0;
    for (int32_t i = 0; i < num_vtx; i++) {
        poly_z += vtx[i].zv;
    }
    poly_z /= num_vtx;

    M_InsertPolyFlat(
        num_points, (double)(poly_z - 0x20000), 0, 0, 0, POLY_HWR_TRANS);
}

static void M_InsertGT3_ZBuffered(
    RENDERER *const renderer, const PHD_VBUF *const vtx0,
    const PHD_VBUF *const vtx1, const PHD_VBUF *const vtx2,
    const OBJECT_TEXTURE *const texture, const TEXTURE_UV *const uv0,
    const TEXTURE_UV *const uv1, const TEXTURE_UV *const uv2)
{
    const int8_t clip_or = vtx0->clip | vtx1->clip | vtx2->clip;
    const int8_t clip_and = vtx0->clip & vtx1->clip & vtx2->clip;
    if (clip_and != 0) {
        return;
    }

    const PHD_VBUF *const vtx[3] = { vtx0, vtx1, vtx2 };
    const TEXTURE_UV *const uv[3] = { uv0, uv1, uv2 };
    int32_t num_points = 3;

    if (clip_or >= 0) {
        if (!VBUF_VISIBLE(*vtx0, *vtx1, *vtx2)) {
            return;
        }

        if (clip_or == 0) {
            for (int32_t i = 0; i < 3; i++) {
                GFX_3D_VERTEX *const vbuf_gl = &m_VBufferGL[i];
                vbuf_gl->x = vtx[i]->xs;
                vbuf_gl->y = vtx[i]->ys;
                vbuf_gl->z = MAKE_DEPTH(vtx[i]);
                vbuf_gl->w = vtx[i]->rhw;
                vbuf_gl->s = (double)uv[i]->u * vtx[i]->rhw / (double)PHD_ONE;
                vbuf_gl->t = (double)uv[i]->v * vtx[i]->rhw / (double)PHD_ONE;
                M_ShadeLight(vbuf_gl, vtx[i]->g, true);
            }

            M_SelectTexture(renderer, texture->tex_page);
            M_EnableColorKey(renderer, texture->draw_type != DRAW_OPAQUE);
            M_DrawPrimitive(renderer, GFX_3D_PRIM_TRI, m_VBufferGL, 3, true);
            return;
        }

        for (int32_t i = 0; i < 3; i++) {
            VERTEX_INFO *const vbuf = &m_VBuffer[i];
            vbuf->x = vtx[i]->xs;
            vbuf->y = vtx[i]->ys;
            vbuf->z = vtx[i]->zv;
            vbuf->rhw = vtx[i]->rhw;
            vbuf->g = (double)vtx[i]->g;
            vbuf->u = (double)uv[i]->u * vtx[i]->rhw;
            vbuf->v = (double)uv[i]->v * vtx[i]->rhw;
        }
    } else {
        if (!Render_VisibleZClip(vtx0, vtx1, vtx2)) {
            return;
        }

        POINT_INFO points[num_points];
        for (int32_t i = 0; i < num_points; i++) {
            points[i].xv = vtx[i]->xv;
            points[i].yv = vtx[i]->yv;
            points[i].zv = vtx[i]->zv;
            points[i].rhw = vtx[i]->rhw;
            points[i].xs = vtx[i]->xs;
            points[i].ys = vtx[i]->ys;
            points[i].g = vtx[i]->g;
            points[i].u = uv[i]->u;
            points[i].v = uv[i]->v;
        }
        num_points = Render_ZedClipper(num_points, points, m_VBuffer);
        if (num_points == 0) {
            return;
        }
    }

    M_SelectTexture(renderer, texture->tex_page);
    M_EnableColorKey(renderer, texture->draw_type != DRAW_OPAQUE);
    M_DrawPolyTextured(renderer, num_points);
}

static void M_InsertGT4_ZBuffered(
    RENDERER *const renderer, const PHD_VBUF *const vtx0,
    const PHD_VBUF *const vtx1, const PHD_VBUF *const vtx2,
    const PHD_VBUF *const vtx3, const OBJECT_TEXTURE *const texture)
{
    const int8_t clip_and = vtx0->clip & vtx1->clip & vtx2->clip & vtx3->clip;
    const int8_t clip_or = vtx0->clip | vtx1->clip | vtx2->clip | vtx3->clip;

    if (clip_and != 0) {
        return;
    }

    if (clip_or >= 0) {
        if (!VBUF_VISIBLE(*vtx0, *vtx1, *vtx2)) {
            return;
        }
    } else if (clip_or < 0) {
        if (!Render_VisibleZClip(vtx0, vtx1, vtx2)) {
            return;
        }
    }

    if (clip_or != 0) {
        M_InsertGT3_ZBuffered(
            renderer, vtx0, vtx1, vtx2, texture, texture->uv, &texture->uv[1],
            &texture->uv[2]);
        M_InsertGT3_ZBuffered(
            renderer, vtx0, vtx2, vtx3, texture, texture->uv, &texture->uv[2],
            &texture->uv[3]);
        return;
    }

    const PHD_VBUF *const vtx[4] = { vtx0, vtx1, vtx2, vtx3 };
    for (int32_t i = 0; i < 4; i++) {
        GFX_3D_VERTEX *const vbuf_gl = &m_VBufferGL[i];
        vbuf_gl->x = vtx[i]->xs;
        vbuf_gl->y = vtx[i]->ys;
        vbuf_gl->z = MAKE_DEPTH(vtx[i]);
        vbuf_gl->w = vtx[i]->rhw;
        vbuf_gl->s = texture->uv[i].u * vtx[i]->rhw / (double)PHD_ONE;
        vbuf_gl->t = texture->uv[i].v * vtx[i]->rhw / (double)PHD_ONE;
        M_ShadeLight(vbuf_gl, vtx[i]->g, true);
    }

    M_SelectTexture(renderer, texture->tex_page);
    M_EnableColorKey(renderer, texture->draw_type != DRAW_OPAQUE);
    M_DrawPrimitive(renderer, GFX_3D_PRIM_TRI, m_VBufferGL, 4, true);
}

static void M_InsertFlatFace3s_ZBuffered(
    RENDERER *const renderer, const FACE3 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    M_SelectTexture(renderer, -1);
    M_EnableColorKey(renderer, false);

    if (num == 0) {
        return;
    }

    for (int32_t i = 0; i < num; i++) {
        int32_t num_points = 3;
        const FACE3 *const face = &faces[i];
        const PHD_VBUF *vtx[3] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
        };

        const int8_t clip_or = vtx[0]->clip | vtx[1]->clip | vtx[2]->clip;
        const int8_t clip_and = vtx[0]->clip & vtx[1]->clip & vtx[2]->clip;

        if (clip_and != 0) {
            continue;
        }

        if (clip_or >= 0) {
            if (!VBUF_VISIBLE(*vtx[0], *vtx[1], *vtx[2])) {
                continue;
            }

            for (int32_t i = 0; i < 3; i++) {
                VERTEX_INFO *const vbuf = &m_VBuffer[i];
                vbuf->x = vtx[i]->xs;
                vbuf->y = vtx[i]->ys;
                vbuf->z = vtx[i]->zv;
                vbuf->rhw = vtx[i]->rhw;
                vbuf->g = vtx[i]->g;
            }
        } else {
            if (!Render_VisibleZClip(vtx[0], vtx[1], vtx[2])) {
                continue;
            }

            POINT_INFO points[3];
            for (int32_t i = 0; i < 3; i++) {
                points[i].xv = vtx[i]->xv;
                points[i].yv = vtx[i]->yv;
                points[i].zv = vtx[i]->zv;
                points[i].rhw = vtx[i]->rhw;
                points[i].xs = vtx[i]->xs;
                points[i].ys = vtx[i]->ys;
                points[i].g = vtx[i]->g;
            }
            num_points = Render_ZedClipper(num_points, points, m_VBuffer);
            if (num_points == 0) {
                continue;
            }
        }

        if (clip_or != 0) {
            num_points = Render_XYGClipper(num_points, m_VBuffer);
        }
        if (num_points == 0) {
            continue;
        }

        const RGB_888 color = Output_GetPaletteColor16(face->palette_idx >> 8);
        M_DrawPolyFlat(renderer, num_points, color.r, color.g, color.b);
    }
}

static void M_InsertFlatFace4s_ZBuffered(
    RENDERER *const renderer, const FACE4 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    M_SelectTexture(renderer, -1);
    M_EnableColorKey(renderer, false);

    if (num == 0) {
        return;
    }

    for (int32_t i = 0; i < num; i++) {
        int32_t num_points = 4;
        const FACE4 *const face = &faces[i];
        const PHD_VBUF *const vtx[4] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
            &g_PhdVBuf[face->vertices[3]],
        };

        const int8_t clip_or =
            vtx[0]->clip | vtx[1]->clip | vtx[2]->clip | vtx[3]->clip;
        const int8_t clip_and =
            vtx[0]->clip & vtx[1]->clip & vtx[2]->clip & vtx[3]->clip;

        if (clip_and != 0) {
            continue;
        }

        if (clip_or >= 0) {
            if (!VBUF_VISIBLE(*vtx[0], *vtx[1], *vtx[2])) {
                continue;
            }

            for (int32_t i = 0; i < 4; i++) {
                VERTEX_INFO *const vbuf = &m_VBuffer[i];
                vbuf->x = vtx[i]->xs;
                vbuf->y = vtx[i]->ys;
                vbuf->z = vtx[i]->zv;
                vbuf->rhw = vtx[i]->rhw;
                vbuf->g = vtx[i]->g;
            }
        } else {
            if (!Render_VisibleZClip(vtx[0], vtx[1], vtx[2])) {
                continue;
            }

            POINT_INFO points[4];
            for (int32_t i = 0; i < 4; i++) {
                points[i].xv = vtx[i]->xv;
                points[i].yv = vtx[i]->yv;
                points[i].zv = vtx[i]->zv;
                points[i].rhw = vtx[i]->rhw;
                points[i].xs = vtx[i]->xs;
                points[i].ys = vtx[i]->ys;
                points[i].g = vtx[i]->g;
            }
            num_points = Render_ZedClipper(num_points, points, m_VBuffer);
            if (num_points == 0) {
                continue;
            }
        }

        if (clip_or != 0) {
            num_points = Render_XYGClipper(num_points, m_VBuffer);
        }
        if (num_points == 0) {
            continue;
        }

        const RGB_888 color = Output_GetPaletteColor16(face->palette_idx >> 8);
        M_DrawPolyFlat(renderer, num_points, color.r, color.g, color.b);
    }
}

static void M_InsertTexturedFace3s_ZBuffered(
    RENDERER *const renderer, const FACE3 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    for (int32_t i = 0; i < num; i++) {
        const FACE3 *const face = &faces[i];
        const PHD_VBUF *const vtx[3] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
        };
        const OBJECT_TEXTURE *const texture =
            Output_GetObjectTexture(face->texture_idx);

        if (texture->draw_type != DRAW_OPAQUE && g_DiscardTransparent) {
            continue;
        }

        const TEXTURE_UV *const uv = texture->uv;
        if (texture->draw_type != DRAW_OPAQUE) {
            M_InsertGT3_Sorted(
                renderer, vtx[0], vtx[1], vtx[2], texture, &uv[0], &uv[1],
                &uv[2], sort_type);
        } else {
            M_InsertGT3_ZBuffered(
                renderer, vtx[0], vtx[1], vtx[2], texture, &uv[0], &uv[1],
                &uv[2]);
        }
    }
}

static void M_InsertTexturedFace4s_ZBuffered(
    RENDERER *const renderer, const FACE4 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    for (int32_t i = 0; i < num; i++) {
        const FACE4 *const face = &faces[i];
        const PHD_VBUF *const vtx[4] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
            &g_PhdVBuf[face->vertices[3]],

        };
        const OBJECT_TEXTURE *const texture =
            Output_GetObjectTexture(face->texture_idx);

        if (texture->draw_type != DRAW_OPAQUE && g_DiscardTransparent) {
            continue;
        }

        const TEXTURE_UV *const uv = texture->uv;
        if (texture->draw_type != DRAW_OPAQUE) {
            M_InsertGT4_Sorted(
                renderer, vtx[0], vtx[1], vtx[2], vtx[3], texture, sort_type);
        } else {
            M_InsertGT4_ZBuffered(
                renderer, vtx[0], vtx[1], vtx[2], vtx[3], texture);
        }
    }
}

static void M_InsertFlatRect_ZBuffered(
    RENDERER *const renderer, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
    int32_t z, const uint8_t color_idx)
{
    if (x2 <= x1 || y2 <= y1) {
        return;
    }

    CLAMPL(x1, 0);
    CLAMPL(y1, 0);
    CLAMPG(x2, g_PhdWinWidth);
    CLAMPG(y2, g_PhdWinHeight);
    CLAMP(z, g_PhdNearZ, g_PhdFarZ);

    const double rhw = g_RhwFactor / (double)z;
    const double sz = MAKE_DEPTH_FROM_RHW(rhw);

    const RGB_888 color = Output_GetPaletteColor8(color_idx);

    m_VBufferGL[0].x = x1;
    m_VBufferGL[0].y = y1;
    m_VBufferGL[1].x = x2;
    m_VBufferGL[1].y = y1;
    m_VBufferGL[2].x = x2;
    m_VBufferGL[2].y = y2;
    m_VBufferGL[3].x = x1;
    m_VBufferGL[3].y = y2;
    for (int32_t i = 0; i < 4; i++) {
        GFX_3D_VERTEX *const vbuf_gl = &m_VBufferGL[i];
        vbuf_gl->z = sz;
        vbuf_gl->w = rhw;
        M_ShadeColor(vbuf_gl, color.r, color.g, color.b, 0xFF);
    }

    M_SelectTexture(renderer, -1);
    M_EnableColorKey(renderer, false);
    M_DrawPrimitive(renderer, GFX_3D_PRIM_TRI, m_VBufferGL, 4, true);
}

static void M_InsertLine_ZBuffered(
    RENDERER *const renderer, const int32_t x1, const int32_t y1,
    const int32_t x2, const int32_t y2, int32_t z, const uint8_t color_idx)
{
    if (z >= g_PhdFarZ) {
        return;
    }
    CLAMPL(z, g_PhdNearZ);

    const double rhw = g_RhwFactor / (double)z;
    const double sz = MAKE_DEPTH_FROM_RHW(rhw);
    const RGB_888 color = Output_GetPaletteColor8(color_idx);

    m_VBufferGL[0].x = x1;
    m_VBufferGL[0].y = y1;
    m_VBufferGL[1].x = x2;
    m_VBufferGL[1].y = y2;
    for (int32_t i = 0; i < 2; i++) {
        GFX_3D_VERTEX *const vbuf_gl = &m_VBufferGL[i];
        vbuf_gl->z = sz;
        vbuf_gl->w = rhw;
        M_ShadeColor(vbuf_gl, color.r, color.g, color.b, 0xFF);
    }

    M_SelectTexture(renderer, -1);
    M_EnableColorKey(renderer, false);
    M_DrawPrimitive(renderer, GFX_3D_PRIM_LINE, m_VBufferGL, 2, true);
}

static void M_ResetFuncPtrs(RENDERER *const renderer)
{
    renderer->InsertTransQuad = M_InsertTransQuad_Sorted;
    renderer->InsertTransOctagon = M_InsertTransOctagon_Sorted;
    renderer->InsertSprite = M_InsertSprite_Sorted;
    if (g_Config.rendering.enable_zbuffer) {
        renderer->InsertFlatFace3s = M_InsertFlatFace3s_ZBuffered;
        renderer->InsertFlatFace4s = M_InsertFlatFace4s_ZBuffered;
        renderer->InsertTexturedFace3s = M_InsertTexturedFace3s_ZBuffered;
        renderer->InsertTexturedFace4s = M_InsertTexturedFace4s_ZBuffered;
        renderer->InsertLine = M_InsertLine_ZBuffered;
        renderer->InsertFlatRect = M_InsertFlatRect_ZBuffered;
    } else {
        renderer->InsertFlatFace3s = M_InsertFlatFace3s_Sorted;
        renderer->InsertFlatFace4s = M_InsertFlatFace4s_Sorted;
        renderer->InsertTexturedFace3s = M_InsertTexturedFace3s_Sorted;
        renderer->InsertTexturedFace4s = M_InsertTexturedFace4s_Sorted;
        renderer->InsertLine = M_InsertLine_Sorted;
        renderer->InsertFlatRect = M_InsertFlatRect_Sorted;
    }
}

static void M_ResetParams(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    GFX_3D_Renderer_SetBrightnessMultiplier(
        priv->renderer_3d,
        g_Config.rendering.lighting_contrast == LIGHTING_CONTRAST_LOW ? 1.0
                                                                      : 2.0);
}

static void M_Init(RENDERER *const renderer)
{
    M_PRIV *const priv = Memory_Alloc(sizeof(M_PRIV));
    priv->renderer_2d = GFX_2D_Renderer_Create();
    priv->renderer_3d = GFX_3D_Renderer_Create();
    priv->current_texture = -1;

    for (int32_t i = 0; i < GFX_MAX_TEXTURES; i++) {
        priv->texture_map[i] = GFX_NO_TEXTURE;
    }
    priv->env_map_texture = GFX_NO_TEXTURE;

    renderer->initialized = true;
    renderer->priv = priv;
    M_ResetParams(renderer);
}

static void M_Shutdown(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    if (!renderer->initialized) {
        return;
    }

    if (priv->renderer_2d == nullptr) {
        GFX_2D_Renderer_Destroy(priv->renderer_2d);
        priv->renderer_2d = nullptr;
    }

    if (priv->renderer_3d == nullptr) {
        GFX_3D_Renderer_Destroy(priv->renderer_3d);
        priv->renderer_3d = nullptr;
    }

    renderer->initialized = false;
}

static void M_Open(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    ASSERT(renderer->initialized);
    if (renderer->open) {
        return;
    }
    if (!renderer->initialized) {
        renderer->Init(renderer);
    }

    memset(m_HWR_VertexBuffer, 0, sizeof(m_HWR_VertexBuffer));

    for (int32_t i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (priv->surface_tex[i] == nullptr) {
            GFX_2D_SURFACE_DESC surface_desc = {
                .width = TEXTURE_PAGE_WIDTH,
                .height = TEXTURE_PAGE_HEIGHT,
            };
            priv->surface_tex[i] = GFX_2D_Surface_Create(&surface_desc);
        }
    }

    M_LoadTexturePages(renderer);
    renderer->open = true;
}

static void M_Close(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    if (!renderer->open) {
        return;
    }

    if (priv->surface_pic != nullptr) {
        GFX_2D_Surface_Free(priv->surface_pic);
        priv->surface_pic = nullptr;
    }
    for (int32_t i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (priv->surface_tex[i] != nullptr) {
            GFX_2D_Surface_Free(priv->surface_tex[i]);
            priv->surface_tex[i] = nullptr;
        }
    }

    M_ReleaseTextures(renderer);
    renderer->open = false;
}

static void M_BeginScene(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    priv->current_texture = -1;
    ASSERT(renderer->initialized && renderer->open);
    M_EnableZBuffer(renderer, true, true);
    GFX_3D_Renderer_RenderBegin(priv->renderer_3d);
    GFX_3D_Renderer_SetAlphaThreshold(priv->renderer_3d, -1.0);
    GFX_3D_Renderer_SetTextureFilter(
        priv->renderer_3d, g_Config.rendering.texture_filter);
}

static void M_EndScene(RENDERER *const renderer)
{
}

static void M_Reset(RENDERER *const renderer, const RENDER_RESET_FLAGS flags)
{
    M_PRIV *const priv = renderer->priv;
    if (!renderer->initialized) {
        return;
    }
    if (flags & (RENDER_RESET_TEXTURES | RENDER_RESET_PALETTE)) {
        LOG_DEBUG("Reloading textures");
        M_LoadTexturePages(renderer);
    }
    if (flags & RENDER_RESET_PARAMS) {
        M_ResetParams(renderer);
    }
    M_ResetFuncPtrs(renderer);
}

static void M_ResetPolyList(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    ASSERT(renderer->initialized && renderer->open);
    m_HWR_VertexPtr = m_HWR_VertexBuffer;
    M_EnableZBuffer(renderer, true, true);
    M_ClearZBuffer(renderer);
    GFX_3D_Renderer_SetAlphaThreshold(priv->renderer_3d, -1.0);
}

static void M_DrawPolyList(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    ASSERT(renderer->initialized && renderer->open);

    Render_SortPolyList();
    // NOTE: depth writes only work for fully transparent pixels
    M_EnableZBuffer(renderer, true, true);
    GFX_3D_Renderer_SetAlphaThreshold(priv->renderer_3d, 0.1);

    for (int32_t i = 0; i < g_SurfaceCount; i++) {
        uint16_t *buf_ptr = (uint16_t *)g_SortBuffer[i]._0;

        uint16_t poly_type = *buf_ptr++;
        uint16_t tex_page =
            (poly_type == POLY_HWR_GTMAP || poly_type == POLY_HWR_WGTMAP)
            ? *buf_ptr++
            : 0;
        uint16_t vtx_count = *buf_ptr++;
        GFX_3D_VERTEX *vtx_ptr = *(GFX_3D_VERTEX **)buf_ptr;

        switch (poly_type) {
        // triangle fan (texture)
        case POLY_HWR_GTMAP:
        // triangle fan (texture + colorkey)
        case POLY_HWR_WGTMAP:
            M_SelectTexture(renderer, tex_page);
            M_EnableColorKey(renderer, poly_type == POLY_HWR_WGTMAP);
            M_DrawPrimitive(
                renderer, GFX_3D_PRIM_TRI, vtx_ptr, vtx_count, true);
            break;

        // triangle fan (color)
        case POLY_HWR_GOURAUD:
            M_SelectTexture(renderer, -1);
            M_EnableColorKey(renderer, false);
            M_DrawPrimitive(
                renderer, GFX_3D_PRIM_TRI, vtx_ptr, vtx_count, true);
            break;

        // line strip (color)
        case POLY_HWR_LINE:
            GFX_3D_Renderer_SetPrimType(priv->renderer_3d, GFX_3D_PRIM_LINE);
            M_SelectTexture(renderer, -1);
            M_EnableColorKey(renderer, false);
            GFX_3D_Renderer_RenderPrimList(
                priv->renderer_3d, vtx_ptr, vtx_count);
            GFX_3D_Renderer_SetPrimType(priv->renderer_3d, GFX_3D_PRIM_TRI);
            break;

        // triangle fan (color + semitransparent)
        case POLY_HWR_TRANS: {
            GFX_3D_Renderer_SetBlendingMode(
                priv->renderer_3d, GFX_BLEND_MODE_NORMAL);
            M_SelectTexture(renderer, -1);
            M_DrawPrimitive(
                renderer, GFX_3D_PRIM_TRI, vtx_ptr, vtx_count, true);
            GFX_3D_Renderer_SetBlendingMode(
                priv->renderer_3d, GFX_BLEND_MODE_OFF);
            break;
        }
        }
    }

    GFX_3D_Renderer_RenderEnd(priv->renderer_3d);
}

static void M_EnableZBuffer(
    RENDERER *const renderer, const bool z_write_enable,
    const bool z_test_enable)
{
    M_PRIV *const priv = renderer->priv;
    GFX_3D_Renderer_SetDepthBufferEnabled(
        priv->renderer_3d, g_Config.rendering.enable_zbuffer);
    GFX_3D_Renderer_SetDepthWritesEnabled(priv->renderer_3d, z_write_enable);
    GFX_3D_Renderer_SetDepthTestEnabled(priv->renderer_3d, z_test_enable);
}

static void M_ClearZBuffer(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    GFX_3D_Renderer_ClearDepth(priv->renderer_3d);
}

void Renderer_HW_Prepare(RENDERER *const renderer)
{
    renderer->Init = M_Init;
    renderer->Open = M_Open;
    renderer->Close = M_Close;
    renderer->Shutdown = M_Shutdown;
    renderer->BeginScene = M_BeginScene;
    renderer->EndScene = M_EndScene;
    renderer->Reset = M_Reset;
    renderer->ResetPolyList = M_ResetPolyList;
    renderer->DrawPolyList = M_DrawPolyList;
    renderer->EnableZBuffer = M_EnableZBuffer;
    renderer->ClearZBuffer = M_ClearZBuffer;
    M_ResetFuncPtrs(renderer);
}
