#include "game/render/swr.h"

#include "decomp/decomp.h"
#include "game/output.h"
#include "game/render/priv.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/debug.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#define MAKE_Q_ID(g) ((g >> 16) & 0xFF)
#define MAKE_TEX_ID(v, u) ((((v >> 16) & 0xFF) << 8) | ((u >> 16) & 0xFF))
#define MAKE_PAL_IDX(c) (c)
#define PIX_FMT uint8_t
#define PIX_FMT_GL GL_UNSIGNED_BYTE
#define ALPHA_FMT uint8_t

typedef enum {
    POLY_GTMAP,
    POLY_WGTMAP,
    POLY_GTMAP_PERSP,
    POLY_WGTMAP_PERSP,
    POLY_LINE,
    POLY_FLAT,
    POLY_GOURAUD,
    POLY_TRANS,
    POLY_SPRITE,
} POLY_TYPE;

typedef struct {
    GFX_2D_RENDERER *renderer_2d;
    GFX_2D_SURFACE *surface;
    GFX_2D_SURFACE *surface_alpha;
    GFX_COLOR palette[256];
} M_PRIV;

#pragma pack(push, 1)
typedef struct {
    uint16_t x;
    uint16_t y;
} XGEN_X;

typedef struct {
    int32_t x1;
    int32_t x2;
} XBUF_X;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t g;
} XGEN_XG;

typedef struct {
    int32_t x1;
    int32_t g1;
    int32_t x2;
    int32_t g2;
} XBUF_XG;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t g;
    uint16_t u;
    uint16_t v;
} XGEN_XGUV;

typedef struct {
    int32_t x1;
    int32_t g1;
    int32_t u1;
    int32_t v1;
    int32_t x2;
    int32_t g2;
    int32_t u2;
    int32_t v2;
} XBUF_XGUV;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t g;
    float rhw;
    float u;
    float v;
} XGEN_XGUVP;

typedef struct {
    int32_t x1;
    int32_t g1;
    float u1;
    float v1;
    float rhw1;
    int32_t x2;
    int32_t g2;
    float u2;
    float v2;
    float rhw2;
} XBUF_XGUVP;
#pragma pack(pop)

static VERTEX_INFO m_VBuffer[32] = {};
static void *m_XBuffer = nullptr;
static int32_t m_XGenY1 = 0;
static int32_t m_XGenY2 = 0;

static void M_FlatA(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *target_surface, int32_t y1,
    int32_t y2, uint8_t color_idx);
static void M_TransA(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *target_surface, int32_t y1,
    int32_t y2, uint8_t depth);
static void M_GourA(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *target_surface, int32_t y1,
    int32_t y2, uint8_t color_idx);
static void M_GTMapA(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *target_surface, int32_t y1,
    int32_t y2, const uint8_t *tex_page);
static void M_WGTMapA(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *target_surface, int32_t y1,
    int32_t y2, const uint8_t *tex_page);
static void M_GTMapPersp32FP(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *target_surface, int32_t y1,
    int32_t y2, const uint8_t *tex_page);
static void M_WGTMapPersp32FP(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *target_surface, int32_t y1,
    int32_t y2, const uint8_t *tex_page);

static bool M_XGenX(const int16_t *obj_ptr);
static bool M_XGenXG(const int16_t *obj_ptr);
static bool M_XGenXGUV(const int16_t *obj_ptr);

static void M_DrawPolyFlat(
    const int16_t *obj_ptr, GFX_2D_SURFACE *target_surface,
    GFX_2D_SURFACE *alpha_surface);
static void M_DrawPolyTrans(
    const int16_t *obj_ptr, GFX_2D_SURFACE *target_surface,
    GFX_2D_SURFACE *alpha_surface);
static void M_DrawPolyGouraud(
    const int16_t *obj_ptr, GFX_2D_SURFACE *target_surface,
    GFX_2D_SURFACE *alpha_surface);
static void M_DrawPolyGTMap(
    const int16_t *obj_ptr, GFX_2D_SURFACE *target_surface,
    GFX_2D_SURFACE *alpha_surface);
static void M_DrawPolyWGTMap(
    const int16_t *obj_ptr, GFX_2D_SURFACE *target_surface,
    GFX_2D_SURFACE *alpha_surface);
static void M_DrawPolyGTMapPersp(
    const int16_t *obj_ptr, GFX_2D_SURFACE *target_surface,
    GFX_2D_SURFACE *alpha_surface);
static void M_DrawPolyWGTMapPersp(
    const int16_t *obj_ptr, GFX_2D_SURFACE *target_surface,
    GFX_2D_SURFACE *alpha_surface);
static void M_DrawPolyLine(
    const int16_t *obj_ptr, GFX_2D_SURFACE *target_surface,
    GFX_2D_SURFACE *alpha_surface);
static void M_DrawScaledSpriteC(
    const int16_t *obj_ptr, GFX_2D_SURFACE *target_surface,
    GFX_2D_SURFACE *alpha_surface);

static void M_InsertFlatFace3s(
    RENDERER *const renderer, const FACE3 *faces, int32_t num,
    SORT_TYPE sort_type);
static void M_InsertFlatFace4s(
    RENDERER *const renderer, const FACE4 *faces, int32_t num,
    SORT_TYPE sort_type);
static void M_InsertTexturedFace3s(
    RENDERER *renderer, const FACE3 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertTexturedFace4s(
    RENDERER *renderer, const FACE4 *faces, int32_t num, SORT_TYPE sort_type);
static void M_InsertLine(
    RENDERER *renderer, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
    int32_t z, uint8_t color_idx);
static void M_InsertFlatRect(
    RENDERER *renderer, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
    int32_t z, uint8_t color_idx);
static void M_InsertTransQuad(
    RENDERER *renderer, int32_t x, int32_t y, int32_t width, int32_t height,
    int32_t z);
static void M_InsertTransOctagon(
    RENDERER *renderer, const PHD_VBUF *vbuf, int16_t shade);
static void M_InsertSprite(
    RENDERER *renderer, int32_t z, int32_t x0, int32_t y0, const int32_t x1,
    int32_t y1, int32_t sprite_idx, const int16_t shade);

static void (*m_PolyDrawRoutines[])(
    const int16_t *, GFX_2D_SURFACE *, GFX_2D_SURFACE *) = {
    // clang-format off
    [POLY_GTMAP]        = M_DrawPolyGTMap,
    [POLY_WGTMAP]       = M_DrawPolyWGTMap,
    [POLY_GTMAP_PERSP]  = M_DrawPolyGTMapPersp,
    [POLY_WGTMAP_PERSP] = M_DrawPolyWGTMapPersp,
    [POLY_LINE]         = M_DrawPolyLine,
    [POLY_FLAT]         = M_DrawPolyFlat,
    [POLY_GOURAUD]      = M_DrawPolyGouraud,
    [POLY_TRANS]        = M_DrawPolyTrans,
    [POLY_SPRITE]       = M_DrawScaledSpriteC,
    // clang-format on
};

static void M_FlatA(
    GFX_2D_SURFACE *const alpha_surface, GFX_2D_SURFACE *const target_surface,
    int32_t y1, int32_t y2, const uint8_t color_idx)
{
    int32_t y_size = y2 - y1;
    if (y_size <= 0) {
        return;
    }

    const XBUF_X *xbuf = (const XBUF_X *)m_XBuffer + y1;
    const int32_t target_stride = target_surface->desc.pitch;
    const int32_t alpha_stride = alpha_surface->desc.pitch;
    PIX_FMT *target_ptr = target_surface->buffer + y1 * target_stride;
    ALPHA_FMT *alpha_ptr = alpha_surface->buffer + y1 * alpha_stride;

    while (y_size > 0) {
        const int32_t x = xbuf->x1 / PHD_ONE;
        const int32_t x_size = (xbuf->x2 / PHD_ONE) - x;
        if (x_size > 0) {
            memset(
                target_ptr + x, MAKE_PAL_IDX(color_idx),
                x_size * sizeof(PIX_FMT));
            memset(alpha_ptr + x, 255, x_size * sizeof(ALPHA_FMT));
        }
        y_size--;
        xbuf++;
        target_ptr += target_stride;
        alpha_ptr += alpha_stride;
    }
}

static void M_TransA(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *const target_surface,
    const int32_t y1, const int32_t y2, const uint8_t depth)
{
    int32_t y_size = y2 - y1;
    if (y_size <= 0 || depth >= LIGHT_MAP_SIZE) {
        return;
    }

    const XBUF_X *xbuf = (const XBUF_X *)m_XBuffer + y1;
    const int32_t target_stride = target_surface->desc.pitch;
    const int32_t alpha_stride = alpha_surface->desc.pitch;
    PIX_FMT *target_ptr = target_surface->buffer + y1 * target_stride;
    ALPHA_FMT *alpha_ptr = alpha_surface->buffer + y1 * alpha_stride;
    const LIGHT_MAP *const map = Output_GetLightMap(depth);

    while (y_size > 0) {
        const int32_t x = xbuf->x1 / PHD_ONE;
        int32_t x_size = (xbuf->x2 / PHD_ONE) - x;
        if (x_size <= 0) {
            goto loop_end;
        }

        PIX_FMT *target_line_ptr = target_ptr + x;
        ALPHA_FMT *alpha_line_ptr = alpha_ptr + x;
        while (x_size > 0) {
            *target_line_ptr = MAKE_PAL_IDX(map->index[*target_line_ptr]);
            target_line_ptr++;
            *alpha_line_ptr++ = 255;
            x_size--;
        }

    loop_end:
        y_size--;
        xbuf++;
        target_ptr += target_stride;
        alpha_ptr += alpha_stride;
    }
}

static void M_GourA(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *const target_surface,
    const int32_t y1, const int32_t y2, const uint8_t color_idx)
{
    int32_t y_size = y2 - y1;
    if (y_size <= 0) {
        return;
    }

    const XBUF_XG *xbuf = (const XBUF_XG *)m_XBuffer + y1;
    const int32_t target_stride = target_surface->desc.pitch;
    const int32_t alpha_stride = alpha_surface->desc.pitch;
    PIX_FMT *target_ptr = target_surface->buffer + y1 * target_stride;
    ALPHA_FMT *alpha_ptr = alpha_surface->buffer + y1 * alpha_stride;
    const SHADE_MAP *const map = Output_GetShadeMap(color_idx);

    while (y_size > 0) {
        const int32_t x = xbuf->x1 / PHD_ONE;
        int32_t x_size = (xbuf->x2 / PHD_ONE) - x;
        if (x_size <= 0) {
            goto loop_end;
        }

        int32_t g = xbuf->g1;
        const int32_t g_add = (xbuf->g2 - g) / x_size;

        PIX_FMT *target_line_ptr = target_ptr + x;
        ALPHA_FMT *alpha_line_ptr = alpha_ptr + x;
        while (x_size > 0) {
            *target_line_ptr++ = MAKE_PAL_IDX(map->index[MAKE_Q_ID(g)]);
            *alpha_line_ptr++ = 255;
            g += g_add;
            x_size--;
        }

    loop_end:
        y_size--;
        xbuf++;
        target_ptr += target_stride;
        alpha_ptr += alpha_stride;
    }
}

static void M_GTMapA(
    GFX_2D_SURFACE *const alpha_surface, GFX_2D_SURFACE *const target_surface,
    const int32_t y1, const int32_t y2, const uint8_t *const tex_page)
{
    int32_t y_size = y2 - y1;
    if (y_size <= 0) {
        return;
    }

    const XBUF_XGUV *xbuf = (const XBUF_XGUV *)m_XBuffer + y1;
    const int32_t target_stride = target_surface->desc.pitch;
    const int32_t alpha_stride = alpha_surface->desc.pitch;
    PIX_FMT *target_ptr = target_surface->buffer + y1 * target_stride;
    ALPHA_FMT *alpha_ptr = alpha_surface->buffer + y1 * alpha_stride;

    while (y_size > 0) {
        const int32_t x = xbuf->x1 / PHD_ONE;
        int32_t x_size = (xbuf->x2 / PHD_ONE) - x;
        if (x_size <= 0) {
            goto loop_end;
        }

        int32_t g = xbuf->g1;
        int32_t u = xbuf->u1;
        int32_t v = xbuf->v1;
        const int32_t g_add = (xbuf->g2 - g) / x_size;
        const int32_t u_add = (xbuf->u2 - u) / x_size;
        const int32_t v_add = (xbuf->v2 - v) / x_size;

        PIX_FMT *target_line_ptr = target_ptr + x;
        ALPHA_FMT *alpha_line_ptr = alpha_ptr + x;
        while (x_size > 0) {
            uint8_t color_idx = tex_page[MAKE_TEX_ID(v, u)];
            *target_line_ptr++ = MAKE_PAL_IDX(
                Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx]);
            *alpha_line_ptr++ = 255;
            g += g_add;
            u += u_add;
            v += v_add;
            x_size--;
        }

    loop_end:
        y_size--;
        xbuf++;
        target_ptr += target_stride;
        alpha_ptr += alpha_stride;
    }
}

static void M_WGTMapA(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *target_surface,
    const int32_t y1, const int32_t y2, const uint8_t *tex_page)
{
    int32_t y_size = y2 - y1;
    if (y_size <= 0) {
        return;
    }

    const XBUF_XGUV *xbuf = (const XBUF_XGUV *)m_XBuffer + y1;
    const int32_t target_stride = target_surface->desc.pitch;
    const int32_t alpha_stride = alpha_surface->desc.pitch;
    PIX_FMT *target_ptr = target_surface->buffer + y1 * target_stride;
    ALPHA_FMT *alpha_ptr = alpha_surface->buffer + y1 * alpha_stride;

    while (y_size > 0) {
        const int32_t x = xbuf->x1 / PHD_ONE;
        int32_t x_size = (xbuf->x2 / PHD_ONE) - x;
        if (x_size <= 0) {
            goto loop_end;
        }

        int32_t g = xbuf->g1;
        int32_t u = xbuf->u1;
        int32_t v = xbuf->v1;
        const int32_t g_add = (xbuf->g2 - g) / x_size;
        const int32_t u_add = (xbuf->u2 - u) / x_size;
        const int32_t v_add = (xbuf->v2 - v) / x_size;

        PIX_FMT *target_line_ptr = target_ptr + x;
        ALPHA_FMT *alpha_line_ptr = alpha_ptr + x;
        while (x_size > 0) {
            const uint8_t color_idx = tex_page[MAKE_TEX_ID(v, u)];
            if (color_idx != 0) {
                *target_line_ptr = MAKE_PAL_IDX(
                    Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx]);
                *alpha_line_ptr = 255;
            }
            target_line_ptr++;
            alpha_line_ptr++;
            g += g_add;
            u += u_add;
            v += v_add;
            x_size--;
        }

    loop_end:
        y_size--;
        xbuf++;
        target_ptr += target_stride;
        alpha_ptr += alpha_stride;
    }
}

static void M_GTMapPersp32FP(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *const target_surface,
    const int32_t y1, const int32_t y2, const uint8_t *const tex_page)
{
    int32_t y_size = y2 - y1;
    if (y_size <= 0) {
        return;
    }

    const XBUF_XGUVP *xbuf = (const XBUF_XGUVP *)m_XBuffer + y1;
    const int32_t target_stride = target_surface->desc.pitch;
    const int32_t alpha_stride = alpha_surface->desc.pitch;
    PIX_FMT *target_ptr = target_surface->buffer + y1 * target_stride;
    ALPHA_FMT *alpha_ptr = alpha_surface->buffer + y1 * alpha_stride;

    while (y_size > 0) {
        const int32_t x = xbuf->x1 / PHD_ONE;
        int32_t x_size = (xbuf->x2 / PHD_ONE) - x;
        if (x_size <= 0) {
            goto loop_end;
        }

        int32_t g = xbuf->g1;
        double u = xbuf->u1;
        double v = xbuf->v1;
        double rhw = xbuf->rhw1;

        const int32_t g_add = (xbuf->g2 - g) / x_size;

        int32_t u0 = PHD_HALF * u / rhw;
        int32_t v0 = PHD_HALF * v / rhw;

        PIX_FMT *target_line_ptr = target_ptr + x;
        ALPHA_FMT *alpha_line_ptr = alpha_ptr + x;
        int32_t batch_size = 32;

        if (x_size >= batch_size) {
            const double u_add =
                (xbuf->u2 - u) / (double)x_size * (double)batch_size;
            const double v_add =
                (xbuf->v2 - v) / (double)x_size * (double)batch_size;
            const double rhw_add =
                (xbuf->rhw2 - rhw) / (double)x_size * (double)batch_size;

            while (x_size >= batch_size) {
                u += u_add;
                v += v_add;
                rhw += rhw_add;

                const int32_t u1 = PHD_HALF * u / rhw;
                const int32_t v1 = PHD_HALF * v / rhw;

                const int32_t u0_add = (u1 - u0) / batch_size;
                const int32_t v0_add = (v1 - v0) / batch_size;

                if ((ABS(u0_add) + ABS(v0_add)) < (PHD_ONE / 2)) {
                    int32_t batch_counter = batch_size / 2;
                    while (batch_counter--) {
                        const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
                        const uint8_t color =
                            Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx];
                        *target_line_ptr++ = MAKE_PAL_IDX(color);
                        *target_line_ptr++ = MAKE_PAL_IDX(color);
                        *alpha_line_ptr++ = 255;
                        *alpha_line_ptr++ = 255;
                        g += g_add * 2;
                        u0 += u0_add * 2;
                        v0 += v0_add * 2;
                    }
                } else {
                    int32_t batch_counter = batch_size;
                    while (batch_counter--) {
                        const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
                        const uint8_t color =
                            Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx];
                        *target_line_ptr++ = MAKE_PAL_IDX(color);
                        *alpha_line_ptr++ = 255;
                        g += g_add;
                        u0 += u0_add;
                        v0 += v0_add;
                    }
                }

                u0 = u1;
                v0 = v1;
                x_size -= batch_size;
            }
        }

        if (x_size > 1) {
            const int32_t u1 = PHD_HALF * xbuf->u2 / xbuf->rhw2;
            const int32_t v1 = PHD_HALF * xbuf->v2 / xbuf->rhw2;
            const int32_t u0_add = (u1 - u0) / x_size;
            const int32_t v0_add = (v1 - v0) / x_size;

            batch_size = x_size & ~1;
            x_size -= batch_size;

            if ((ABS(u0_add) + ABS(v0_add)) < (PHD_ONE / 2)) {
                int32_t batch_counter = batch_size / 2;
                while (batch_counter--) {
                    const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
                    const uint8_t color =
                        Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx];
                    *target_line_ptr++ = MAKE_PAL_IDX(color);
                    *target_line_ptr++ = MAKE_PAL_IDX(color);
                    *alpha_line_ptr++ = 255;
                    *alpha_line_ptr++ = 255;
                    g += g_add * 2;
                    u0 += u0_add * 2;
                    v0 += v0_add * 2;
                }
            } else {
                int32_t batch_counter = batch_size;
                while (batch_counter--) {
                    const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
                    const uint8_t color =
                        Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx];
                    *target_line_ptr++ = MAKE_PAL_IDX(color);
                    *alpha_line_ptr++ = 255;
                    g += g_add;
                    u0 += u0_add;
                    v0 += v0_add;
                }
            }
        }

        if (x_size == 1) {
            const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
            const uint8_t color =
                Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx];
            *target_line_ptr = MAKE_PAL_IDX(color);
            *alpha_line_ptr = 255;
        }

    loop_end:
        y_size--;
        xbuf++;
        target_ptr += target_stride;
        alpha_ptr += alpha_stride;
    }
}

static void M_WGTMapPersp32FP(
    GFX_2D_SURFACE *alpha_surface, GFX_2D_SURFACE *const target_surface,
    const int32_t y1, const int32_t y2, const uint8_t *const tex_page)
{
    int32_t y_size = y2 - y1;
    if (y_size <= 0) {
        return;
    }

    const XBUF_XGUVP *xbuf = (const XBUF_XGUVP *)m_XBuffer + y1;
    const int32_t target_stride = target_surface->desc.pitch;
    const int32_t alpha_stride = alpha_surface->desc.pitch;
    PIX_FMT *target_ptr = target_surface->buffer + y1 * target_stride;
    ALPHA_FMT *alpha_ptr = alpha_surface->buffer + y1 * alpha_stride;

    while (y_size > 0) {
        const int32_t x = xbuf->x1 / PHD_ONE;
        int32_t x_size = (xbuf->x2 / PHD_ONE) - x;
        if (x_size <= 0) {
            goto loop_end;
        }

        int32_t g = xbuf->g1;
        double u = xbuf->u1;
        double v = xbuf->v1;
        double rhw = xbuf->rhw1;

        const int32_t g_add = (xbuf->g2 - g) / x_size;

        int32_t u0 = PHD_HALF * u / rhw;
        int32_t v0 = PHD_HALF * v / rhw;

        PIX_FMT *target_line_ptr = target_ptr + x;
        ALPHA_FMT *alpha_line_ptr = alpha_ptr + x;
        int32_t batch_size = 32;

        if (x_size >= batch_size) {
            const double u_add =
                (xbuf->u2 - u) / (double)x_size * (double)batch_size;
            const double v_add =
                (xbuf->v2 - v) / (double)x_size * (double)batch_size;
            const double rhw_add =
                (xbuf->rhw2 - rhw) / (double)x_size * (double)batch_size;

            while (x_size >= batch_size) {
                u += u_add;
                v += v_add;
                rhw += rhw_add;

                const int32_t u1 = PHD_HALF * u / rhw;
                const int32_t v1 = PHD_HALF * v / rhw;

                const int32_t u0_add = (u1 - u0) / batch_size;
                const int32_t v0_add = (v1 - v0) / batch_size;

                if ((ABS(u0_add) + ABS(v0_add)) < (PHD_ONE / 2)) {
                    int32_t batch_counter = batch_size / 2;
                    while (batch_counter--) {
                        const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
                        if (color_idx != 0) {
                            const uint8_t color =
                                Output_GetLightMap(MAKE_Q_ID(g))
                                    ->index[color_idx];
                            target_line_ptr[0] = MAKE_PAL_IDX(color);
                            target_line_ptr[1] = MAKE_PAL_IDX(color);
                            alpha_line_ptr[0] = 255;
                            alpha_line_ptr[1] = 255;
                        }
                        target_line_ptr += 2;
                        alpha_line_ptr += 2;
                        g += g_add * 2;
                        u0 += u0_add * 2;
                        v0 += v0_add * 2;
                    }
                } else {
                    int32_t batch_counter = batch_size;
                    while (batch_counter--) {
                        const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
                        if (color_idx != 0) {
                            const uint8_t color =
                                Output_GetLightMap(MAKE_Q_ID(g))
                                    ->index[color_idx];
                            *target_line_ptr = MAKE_PAL_IDX(color);
                            *alpha_line_ptr = 255;
                        }
                        target_line_ptr++;
                        alpha_line_ptr++;
                        g += g_add;
                        u0 += u0_add;
                        v0 += v0_add;
                    }
                }

                u0 = u1;
                v0 = v1;
                x_size -= batch_size;
            }
        }

        if (x_size > 1) {
            const int32_t u1 = PHD_HALF * xbuf->u2 / xbuf->rhw2;
            const int32_t v1 = PHD_HALF * xbuf->v2 / xbuf->rhw2;
            const int32_t u0_add = (u1 - u0) / x_size;
            const int32_t v0_add = (v1 - v0) / x_size;

            batch_size = x_size & ~1;
            x_size -= batch_size;

            if ((ABS(u0_add) + ABS(v0_add)) < (PHD_ONE / 2)) {
                int32_t batch_counter = batch_size / 2;
                while (batch_counter--) {
                    const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
                    if (color_idx != 0) {
                        const uint8_t color =
                            Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx];
                        target_line_ptr[0] = MAKE_PAL_IDX(color);
                        target_line_ptr[1] = MAKE_PAL_IDX(color);
                        alpha_line_ptr[0] = 255;
                        alpha_line_ptr[1] = 255;
                    }
                    target_line_ptr += 2;
                    alpha_line_ptr += 2;
                    g += g_add * 2;
                    u0 += u0_add * 2;
                    v0 += v0_add * 2;
                };
            } else {
                int32_t batch_counter = batch_size;
                while (batch_counter--) {
                    const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
                    if (color_idx != 0) {
                        const uint8_t color =
                            Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx];
                        *target_line_ptr = MAKE_PAL_IDX(color);
                        *alpha_line_ptr = 255;
                    }
                    target_line_ptr++;
                    alpha_line_ptr++;
                    g += g_add;
                    u0 += u0_add;
                    v0 += v0_add;
                }
            }
        }

        if (x_size == 1) {
            const uint8_t color_idx = tex_page[MAKE_TEX_ID(v0, u0)];
            if (color_idx != 0) {
                const uint8_t color =
                    Output_GetLightMap(MAKE_Q_ID(g))->index[color_idx];
                *target_line_ptr = MAKE_PAL_IDX(color);
                *alpha_line_ptr = 255;
            }
        }

    loop_end:
        y_size--;
        xbuf++;
        target_ptr += target_stride;
        alpha_ptr += alpha_stride;
    }
}

static bool M_XGenX(const int16_t *obj_ptr)
{
    int32_t pt_count = *obj_ptr++;
    const XGEN_X *pt2 = (const XGEN_X *)obj_ptr;
    const XGEN_X *pt1 = pt2 + (pt_count - 1);

    int32_t y_min = pt1->y;
    int32_t y_max = pt1->y;

    while (pt_count--) {
        const int32_t x1 = pt1->x;
        const int32_t y1 = pt1->y;
        const int32_t x2 = pt2->x;
        const int32_t y2 = pt2->y;
        pt1 = pt2++;

        if (y1 < y2) {
            CLAMPG(y_min, y1);
            const int32_t x_size = x2 - x1;
            int32_t y_size = y2 - y1;

            XBUF_X *x_ptr = (XBUF_X *)m_XBuffer + y1;
            const int32_t x_add = PHD_ONE * x_size / y_size;
            int32_t x = x1 * PHD_ONE + (PHD_ONE - 1);

            while (y_size--) {
                x += x_add;
                x_ptr->x2 = x;
                x_ptr++;
            }
        } else if (y2 < y1) {
            CLAMPL(y_max, y1);
            const int32_t x_size = x1 - x2;
            int32_t y_size = y1 - y2;

            XBUF_X *x_ptr = (XBUF_X *)m_XBuffer + y2;
            const int32_t x_add = PHD_ONE * x_size / y_size;
            int32_t x = x2 * PHD_ONE + 1;

            while (y_size--) {
                x += x_add;
                x_ptr->x1 = x;
                x_ptr++;
            }
        }
    }

    if (y_min == y_max) {
        return false;
    }

    m_XGenY1 = y_min;
    m_XGenY2 = y_max;
    return true;
}

static bool M_XGenXG(const int16_t *obj_ptr)
{
    int32_t pt_count = *obj_ptr++;
    const XGEN_XG *pt2 = (const XGEN_XG *)obj_ptr;
    const XGEN_XG *pt1 = pt2 + (pt_count - 1);

    int32_t y_min = pt1->y;
    int32_t y_max = pt1->y;

    while (pt_count--) {
        const int32_t x1 = pt1->x;
        const int32_t y1 = pt1->y;
        const int32_t g1 = pt1->g;
        const int32_t x2 = pt2->x;
        const int32_t y2 = pt2->y;
        const int32_t g2 = pt2->g;
        pt1 = pt2++;

        if (y1 < y2) {
            CLAMPG(y_min, y1);
            const int32_t g_size = g2 - g1;
            const int32_t x_size = x2 - x1;
            int32_t y_size = y2 - y1;

            XBUF_XG *xg_ptr = (XBUF_XG *)m_XBuffer + y1;
            const int32_t x_add = PHD_ONE * x_size / y_size;
            const int32_t g_add = PHD_HALF * g_size / y_size;
            int32_t x = x1 * PHD_ONE + (PHD_ONE - 1);
            int32_t g = g1 * PHD_HALF;

            while (y_size--) {
                x += x_add;
                g += g_add;
                xg_ptr->x2 = x;
                xg_ptr->g2 = g;
                xg_ptr++;
            }
        } else if (y2 < y1) {
            CLAMPL(y_max, y1);
            const int32_t g_size = g1 - g2;
            const int32_t x_size = x1 - x2;
            int32_t y_size = y1 - y2;

            XBUF_XG *xg_ptr = (XBUF_XG *)m_XBuffer + y2;
            const int32_t x_add = PHD_ONE * x_size / y_size;
            const int32_t g_add = PHD_HALF * g_size / y_size;
            int32_t x = x2 * PHD_ONE + 1;
            int32_t g = g2 * PHD_HALF;

            while (y_size--) {
                x += x_add;
                g += g_add;
                xg_ptr->x1 = x;
                xg_ptr->g1 = g;
                xg_ptr++;
            }
        }
    }

    if (y_min == y_max) {
        return false;
    }

    m_XGenY1 = y_min;
    m_XGenY2 = y_max;
    return true;
}

static bool M_XGenXGUV(const int16_t *obj_ptr)
{
    int32_t pt_count = *obj_ptr++;
    const XGEN_XGUV *pt2 = (const XGEN_XGUV *)obj_ptr;
    const XGEN_XGUV *pt1 = pt2 + (pt_count - 1);

    int32_t y_min = pt1->y;
    int32_t y_max = pt1->y;

    while (pt_count--) {
        const int32_t x1 = pt1->x;
        const int32_t y1 = pt1->y;
        const int32_t g1 = pt1->g;
        const int32_t u1 = pt1->u;
        const int32_t v1 = pt1->v;
        const int32_t x2 = pt2->x;
        const int32_t y2 = pt2->y;
        const int32_t g2 = pt2->g;
        const int32_t u2 = pt2->u;
        const int32_t v2 = pt2->v;
        pt1 = pt2++;

        if (y1 < y2) {
            CLAMPG(y_min, y1);
            const int32_t g_size = g2 - g1;
            const int32_t u_size = u2 - u1;
            const int32_t v_size = v2 - v1;
            const int32_t x_size = x2 - x1;
            int32_t y_size = y2 - y1;

            XBUF_XGUV *xguv_ptr = (XBUF_XGUV *)m_XBuffer + y1;
            const int32_t x_add = PHD_ONE * x_size / y_size;
            const int32_t g_add = PHD_HALF * g_size / y_size;
            const int32_t u_add = PHD_HALF * u_size / y_size;
            const int32_t v_add = PHD_HALF * v_size / y_size;
            int32_t x = x1 * PHD_ONE + (PHD_ONE - 1);
            int32_t g = g1 * PHD_HALF;
            int32_t u = u1 * PHD_HALF;
            int32_t v = v1 * PHD_HALF;

            while (y_size--) {
                x += x_add;
                g += g_add;
                u += u_add;
                v += v_add;
                xguv_ptr->x2 = x;
                xguv_ptr->g2 = g;
                xguv_ptr->u2 = u;
                xguv_ptr->v2 = v;
                xguv_ptr++;
            }
        } else if (y2 < y1) {
            CLAMPL(y_max, y1);
            const int32_t g_size = g1 - g2;
            const int32_t u_size = u1 - u2;
            const int32_t v_size = v1 - v2;
            const int32_t x_size = x1 - x2;
            int32_t y_size = y1 - y2;

            XBUF_XGUV *xguv_ptr = (XBUF_XGUV *)m_XBuffer + y2;
            const int32_t x_add = PHD_ONE * x_size / y_size;
            const int32_t g_add = PHD_HALF * g_size / y_size;
            const int32_t u_add = PHD_HALF * u_size / y_size;
            const int32_t v_add = PHD_HALF * v_size / y_size;
            int32_t x = x2 * PHD_ONE + 1;
            int32_t g = g2 * PHD_HALF;
            int32_t u = u2 * PHD_HALF;
            int32_t v = v2 * PHD_HALF;

            while (y_size--) {
                x += x_add;
                g += g_add;
                u += u_add;
                v += v_add;
                xguv_ptr->x1 = x;
                xguv_ptr->g1 = g;
                xguv_ptr->u1 = u;
                xguv_ptr->v1 = v;
                xguv_ptr++;
            }
        }
    }

    if (y_min == y_max) {
        return false;
    }

    m_XGenY1 = y_min;
    m_XGenY2 = y_max;
    return true;
}

static bool M_XGenXGUVPerspFP(const int16_t *obj_ptr)
{
    int32_t pt_count = *obj_ptr++;
    const XGEN_XGUVP *pt2 = (const XGEN_XGUVP *)obj_ptr;
    const XGEN_XGUVP *pt1 = pt2 + (pt_count - 1);

    int32_t y_min = pt1->y;
    int32_t y_max = pt1->y;

    while (pt_count--) {
        const int32_t x1 = pt1->x;
        const int32_t y1 = pt1->y;
        const int32_t g1 = pt1->g;
        const float u1 = pt1->u;
        const float v1 = pt1->v;
        const float rhw1 = pt1->rhw;

        const int32_t x2 = pt2->x;
        const int32_t y2 = pt2->y;
        const int32_t g2 = pt2->g;
        const float u2 = pt2->u;
        const float v2 = pt2->v;
        const float rhw2 = pt2->rhw;

        pt1 = pt2++;

        if (y1 < y2) {
            CLAMPG(y_min, y1);
            const int32_t g_size = g2 - g1;
            const float u_size = u2 - u1;
            const float v_size = v2 - v1;
            const float rhw_size = rhw2 - rhw1;
            const int32_t x_size = x2 - x1;
            int32_t y_size = y2 - y1;

            XBUF_XGUVP *xguv_ptr = (XBUF_XGUVP *)m_XBuffer + y1;
            const int32_t x_add = PHD_ONE * x_size / y_size;
            const int32_t g_add = PHD_HALF * g_size / y_size;
            const float u_add = u_size / (float)y_size;
            const float v_add = v_size / (float)y_size;
            const float rhw_add = rhw_size / (float)y_size;
            int32_t x = x1 * PHD_ONE + (PHD_ONE - 1);
            int32_t g = g1 * PHD_HALF;
            float u = u1;
            float v = v1;
            float rhw = rhw1;

            while (y_size--) {
                x += x_add;
                g += g_add;
                u += u_add;
                v += v_add;
                rhw += rhw_add;
                xguv_ptr->x2 = x;
                xguv_ptr->g2 = g;
                xguv_ptr->u2 = u;
                xguv_ptr->v2 = v;
                xguv_ptr->rhw2 = rhw;
                xguv_ptr++;
            }
        } else if (y2 < y1) {
            CLAMPL(y_max, y1);
            const int32_t g_size = g1 - g2;
            const float u_size = u1 - u2;
            const float v_size = v1 - v2;
            const float rhw_size = rhw1 - rhw2;
            const int32_t x_size = x1 - x2;
            int32_t y_size = y1 - y2;

            XBUF_XGUVP *xguv_ptr = (XBUF_XGUVP *)m_XBuffer + y2;
            const int32_t x_add = PHD_ONE * x_size / y_size;
            const int32_t g_add = PHD_HALF * g_size / y_size;
            const float u_add = u_size / (float)y_size;
            const float v_add = v_size / (float)y_size;
            const float rhw_add = rhw_size / (float)y_size;
            int32_t x = x2 * PHD_ONE + 1;
            int32_t g = g2 * PHD_HALF;
            float u = u2;
            float v = v2;
            float rhw = rhw2;

            while (y_size--) {
                x += x_add;
                g += g_add;
                u += u_add;
                v += v_add;
                rhw += rhw_add;
                xguv_ptr->x1 = x;
                xguv_ptr->g1 = g;
                xguv_ptr->u1 = u;
                xguv_ptr->v1 = v;
                xguv_ptr->rhw1 = rhw;
                xguv_ptr++;
            }
        }
    }

    if (y_min == y_max) {
        return false;
    }

    m_XGenY1 = y_min;
    m_XGenY2 = y_max;
    return true;
}

static void M_DrawPolyFlat(
    const int16_t *const obj_ptr, GFX_2D_SURFACE *const target_surface,
    GFX_2D_SURFACE *const alpha_surface)
{
    if (M_XGenX(obj_ptr + 1)) {
        M_FlatA(alpha_surface, target_surface, m_XGenY1, m_XGenY2, *obj_ptr);
    }
}

static void M_DrawPolyTrans(
    const int16_t *const obj_ptr, GFX_2D_SURFACE *const target_surface,
    GFX_2D_SURFACE *const alpha_surface)
{
    if (M_XGenX(obj_ptr + 1)) {
        M_TransA(alpha_surface, target_surface, m_XGenY1, m_XGenY2, *obj_ptr);
    }
}

static void M_DrawPolyGouraud(
    const int16_t *const obj_ptr, GFX_2D_SURFACE *const target_surface,
    GFX_2D_SURFACE *const alpha_surface)
{
    if (M_XGenXG(obj_ptr + 1)) {
        M_GourA(alpha_surface, target_surface, m_XGenY1, m_XGenY2, *obj_ptr);
    }
}

static void M_DrawPolyGTMap(
    const int16_t *const obj_ptr, GFX_2D_SURFACE *const target_surface,
    GFX_2D_SURFACE *const alpha_surface)
{
    if (M_XGenXGUV(obj_ptr + 1)) {
        M_GTMapA(
            alpha_surface, target_surface, m_XGenY1, m_XGenY2,
            Output_GetTexturePage8(*obj_ptr));
    }
}

static void M_DrawPolyWGTMap(
    const int16_t *const obj_ptr, GFX_2D_SURFACE *const target_surface,
    GFX_2D_SURFACE *const alpha_surface)
{
    if (M_XGenXGUV(obj_ptr + 1)) {
        M_WGTMapA(
            alpha_surface, target_surface, m_XGenY1, m_XGenY2,
            Output_GetTexturePage8(*obj_ptr));
    }
}

static void M_DrawPolyGTMapPersp(
    const int16_t *const obj_ptr, GFX_2D_SURFACE *const target_surface,
    GFX_2D_SURFACE *const alpha_surface)
{
    if (M_XGenXGUVPerspFP(obj_ptr + 1)) {
        M_GTMapPersp32FP(
            alpha_surface, target_surface, m_XGenY1, m_XGenY2,
            Output_GetTexturePage8(*obj_ptr));
    }
}

static void M_DrawPolyWGTMapPersp(
    const int16_t *const obj_ptr, GFX_2D_SURFACE *const target_surface,
    GFX_2D_SURFACE *const alpha_surface)
{
    if (M_XGenXGUVPerspFP(obj_ptr + 1)) {
        M_WGTMapPersp32FP(
            alpha_surface, target_surface, m_XGenY1, m_XGenY2,
            Output_GetTexturePage8(*obj_ptr));
    }
}

static void M_DrawPolyLine(
    const int16_t *obj_ptr, GFX_2D_SURFACE *const target_surface,
    GFX_2D_SURFACE *const alpha_surface)
{
    int32_t x1 = *obj_ptr++;
    int32_t y1 = *obj_ptr++;
    int32_t x2 = *obj_ptr++;
    int32_t y2 = *obj_ptr++;
    uint8_t lcolor = (uint8_t)*obj_ptr;
    const int32_t target_stride = target_surface->desc.pitch;

    if (x2 < x1) {
        int32_t tmp;
        SWAP(x1, x2, tmp);
        SWAP(y1, y2, tmp);
    }

    if (x2 < 0 || x1 > g_PhdWinMaxX) {
        return;
    }

    if (x1 < 0) {
        y1 -= x1 * (y2 - y1) / (x2 - x1);
        x1 = 0;
    }

    if (x2 > g_PhdWinMaxX) {
        y2 = y1 + (y2 - y1) * (g_PhdWinMaxX - x1) / (x2 - x1);
        x2 = g_PhdWinMaxX;
    }

    if (y2 < y1) {
        int32_t tmp;
        SWAP(x1, x2, tmp);
        SWAP(y1, y2, tmp);
    }

    if (y2 < 0 || y1 > g_PhdWinMaxY) {
        return;
    }

    if (y1 < 0) {
        x1 -= y1 * (x2 - x1) / (y2 - y1);
        y1 = 0;
    }

    if (y2 > g_PhdWinMaxY) {
        x2 = x1 + (x2 - x1) * (g_PhdWinMaxY - y1) / (y2 - y1);
        y2 = g_PhdWinMaxY;
    }

    int32_t x_size = x2 - x1;
    int32_t y_size = y2 - y1;
    PIX_FMT *target_ptr = &target_surface->buffer[x1 + target_stride * y1];
    ALPHA_FMT *alpha_ptr = &alpha_surface->buffer[x1 + target_stride * y1];

    if (!x_size && !y_size) {
        *target_ptr = lcolor;
        //*alpha_ptr = 255;
        return;
    }

    int32_t x_add = 0;
    if (x_size < 0) {
        x_add = -1;
        x_size = -x_size;
    } else {
        x_add = 1;
    }

    int32_t y_add;
    if (y_size < 0) {
        y_add = -target_stride;
        y_size = -y_size;
    } else {
        y_add = target_stride;
    }

    int32_t col_add;
    int32_t row_add;
    int32_t cols;
    int32_t rows;
    if (x_size >= y_size) {
        col_add = x_add;
        row_add = y_add;
        cols = x_size + 1;
        rows = y_size + 1;
    } else {
        col_add = y_add;
        row_add = x_add;
        cols = y_size + 1;
        rows = x_size + 1;
    }

    int32_t part_sum = 0;
    int32_t part = PHD_ONE * rows / cols;
    for (int32_t i = 0; i < cols; i++) {
        part_sum += part;
        *target_ptr = lcolor;
        target_ptr += col_add;
        //*alpha_ptr = 255;
        // alpha_ptr += col_add;
        if (part_sum >= PHD_ONE) {
            target_ptr += row_add;
            // alpha_ptr += row_add;
            part_sum -= PHD_ONE;
        }
    }
}

static void M_DrawScaledSpriteC(
    const int16_t *const obj_ptr, GFX_2D_SURFACE *const target_surface,
    GFX_2D_SURFACE *const alpha_surface)
{
    int32_t x0 = obj_ptr[0];
    int32_t y0 = obj_ptr[1];
    int32_t x1 = obj_ptr[2];
    int32_t y1 = obj_ptr[3];
    const int16_t sprite_idx = obj_ptr[4];
    const int16_t shade = obj_ptr[5];

    if (x0 >= x1 || y0 >= y1 || x1 <= 0 || y1 <= 0 || x0 >= g_PhdWinMaxX
        || y0 >= g_PhdWinMaxY) {
        return;
    }

    const LIGHT_MAP *const map = Output_GetLightMap(shade >> 8);
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);

    int32_t u_base = 0x4000;
    int32_t v_base = 0x4000;
    const int32_t u_add = ((sprite->width - 64) << 8) / (x1 - x0);
    const int32_t v_add = ((sprite->height - 64) << 8) / (y1 - y0);

    if (x0 < 0) {
        u_base -= x0 * u_add;
        x0 = 0;
    }
    if (y0 < 0) {
        v_base -= y0 * v_add;
        y0 = 0;
    }
    CLAMPG(x1, g_PhdWinMaxX + 1);
    CLAMPG(y1, g_PhdWinMaxY + 1);

    const int32_t target_stride = target_surface->desc.pitch;
    const int32_t width = x1 - x0;
    const int32_t height = y1 - y0;

    const uint8_t *const page = Output_GetTexturePage8(sprite->tex_page);
    const uint8_t *const src_base = &page[sprite->offset];
    PIX_FMT *target_ptr = &target_surface->buffer[y0 * target_stride + x0];
    ALPHA_FMT *alpha_ptr = &alpha_surface->buffer[y0 * target_stride + x0];
    const int32_t dst_add = target_stride - width;

    const bool use_light_map = map != Output_GetLightMap(16);

    for (int32_t i = 0; i < height; i++) {
        int32_t u = u_base;
        const uint8_t *const src = &src_base[(v_base >> 16) << 8];
        for (int32_t j = 0; j < width; j++) {
            const uint8_t pix = src[u >> 16];
            if (pix != 0) {
                *target_ptr = use_light_map ? map->index[pix] : pix;
                *alpha_ptr = 255;
            }
            u += u_add;
            target_ptr++;
            alpha_ptr++;
        }
        target_ptr += dst_add;
        alpha_ptr += dst_add;
        v_base += v_add;
    }
}

static void M_Init(RENDERER *const renderer)
{
    M_PRIV *const priv = Memory_Alloc(sizeof(M_PRIV));
    priv->renderer_2d = GFX_2D_Renderer_Create();
    renderer->priv = priv;
    renderer->initialized = true;
}

static void M_Open(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    ASSERT(renderer->initialized);
    if (renderer->open) {
        return;
    }

    m_XBuffer = Memory_Realloc(m_XBuffer, sizeof(XBUF_XGUVP) * g_PhdWinHeight);

    {
        GFX_2D_Surface_Free(priv->surface);
        GFX_2D_SURFACE_DESC surface_desc = {
            .width = g_PhdWinWidth,
            .height = g_PhdWinHeight,
            .pitch = g_PhdWinWidth * sizeof(PIX_FMT),
            .bit_count = sizeof(PIX_FMT) * 8,
            .tex_format = GL_RED,
            .tex_type = PIX_FMT_GL,
        };
        priv->surface = GFX_2D_Surface_Create(&surface_desc);
        ASSERT(priv->surface != nullptr);
    }

    {
        GFX_2D_SURFACE_DESC surface_desc = {
            .width = g_PhdWinWidth,
            .height = g_PhdWinHeight,
            .pitch = g_PhdWinWidth,
            .bit_count = 8,
            .tex_format = GL_RED,
            .tex_type = GL_UNSIGNED_BYTE,
        };
        priv->surface_alpha = GFX_2D_Surface_Create(&surface_desc);
        ASSERT(priv->surface_alpha != nullptr);
    }

    renderer->open = true;
}

static void M_Close(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    if (!renderer->initialized || !renderer->open) {
        return;
    }

    Memory_FreePointer(&m_XBuffer);

    if (priv->surface != nullptr) {
        GFX_2D_Surface_Free(priv->surface);
        priv->surface = nullptr;
    }

    if (priv->surface_alpha != nullptr) {
        GFX_2D_Surface_Free(priv->surface_alpha);
        priv->surface_alpha = nullptr;
    }

    renderer->open = false;
}

static void M_Shutdown(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    if (!renderer->initialized) {
        return;
    }
    if (priv->renderer_2d != nullptr) {
        GFX_2D_Renderer_Destroy(priv->renderer_2d);
        priv->renderer_2d = nullptr;
    }
    renderer->initialized = false;
}

static void M_Reset(RENDERER *const renderer, const RENDER_RESET_FLAGS flags)
{
    M_PRIV *const priv = renderer->priv;
    if (!renderer->initialized) {
        return;
    }

    if (flags & RENDER_RESET_PALETTE) {
        for (int32_t i = 0; i < 256; i++) {
            const RGB_888 color = Output_GetPaletteColor8(i);
            priv->palette[i].r = color.r;
            priv->palette[i].g = color.g;
            priv->palette[i].b = color.b;
        }
        GFX_2D_Renderer_SetPalette(priv->renderer_2d, priv->palette);
    }
}

static void M_BeginScene(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    ASSERT(renderer->initialized);
    ASSERT(renderer->open);

    memset(
        priv->surface_alpha->buffer, 0,
        priv->surface_alpha->desc.pitch * priv->surface_alpha->desc.height);
}

static void M_EndScene(RENDERER *const renderer)
{
}

static void M_ResetPolyList(RENDERER *const renderer)
{
}

static void M_DrawPolyList(RENDERER *const renderer)
{
    M_PRIV *const priv = renderer->priv;
    ASSERT(renderer->initialized);
    ASSERT(renderer->open);
    ASSERT(priv->surface != nullptr);

    Render_SortPolyList();

    for (int32_t i = 0; i < g_SurfaceCount; i++) {
        const int16_t *obj_ptr = (const int16_t *)g_SortBuffer[i]._0;
        const int16_t poly_type = *obj_ptr++;
        m_PolyDrawRoutines[poly_type](
            obj_ptr, priv->surface, priv->surface_alpha);
    }

    GFX_2D_Renderer_UploadSurface(priv->renderer_2d, priv->surface);
    GFX_2D_Renderer_UploadAlphaSurface(priv->renderer_2d, priv->surface_alpha);
    GFX_2D_Renderer_Render(priv->renderer_2d);
}

static void M_SetWet(RENDERER *const renderer, const bool is_wet)
{
    M_PRIV *const priv = renderer->priv;
    if (is_wet) {
        const RGB_F tint = Output_GetTint();
        GFX_2D_Renderer_SetTint(
            priv->renderer_2d,
            (GFX_COLOR) {
                .r = tint.r * 255, .g = tint.g * 255, .b = tint.b * 255 });
    } else {
        GFX_2D_Renderer_SetTint(
            priv->renderer_2d, (GFX_COLOR) { .r = 255, .g = 255, .b = 255 });
    }
}

static void M_InsertFlatFace3s(
    RENDERER *const renderer, const FACE3 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    for (int32_t i = 0; i < num; i++) {
        int32_t num_points = 3;
        const FACE3 *const face = &faces[i];
        const PHD_VBUF *const vtx[3] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
        };

        int8_t clip_or = vtx[0]->clip | vtx[1]->clip | vtx[2]->clip;
        int8_t clip_and = vtx[0]->clip & vtx[1]->clip & vtx[2]->clip;

        if (clip_and != 0) {
            continue;
        }

        if (clip_or >= 0) {
            if (!VBUF_VISIBLE(*vtx[0], *vtx[1], *vtx[2])) {
                continue;
            }

            m_VBuffer[0].pos.x = vtx[0]->xs;
            m_VBuffer[0].pos.y = vtx[0]->ys;
            m_VBuffer[0].rhw = vtx[0]->rhw;
            m_VBuffer[0].g = vtx[0]->g;

            m_VBuffer[1].pos.x = vtx[1]->xs;
            m_VBuffer[1].pos.y = vtx[1]->ys;
            m_VBuffer[1].rhw = vtx[1]->rhw;
            m_VBuffer[1].g = vtx[1]->g;

            m_VBuffer[2].pos.x = vtx[2]->xs;
            m_VBuffer[2].pos.y = vtx[2]->ys;
            m_VBuffer[2].rhw = vtx[2]->rhw;
            m_VBuffer[2].g = vtx[2]->g;
        } else {
            if (!Render_VisibleZClip(vtx[0], vtx[1], vtx[2])) {
                continue;
            }

            POINT_INFO points[3] = {
                {
                    .xv = vtx[0]->xv,
                    .yv = vtx[0]->yv,
                    .zv = vtx[0]->zv,
                    .rhw = vtx[0]->rhw,
                    .xs = vtx[0]->xs,
                    .ys = vtx[0]->ys,
                    .g = vtx[0]->g,
                },
                {
                    .xv = vtx[1]->xv,
                    .yv = vtx[1]->yv,
                    .zv = vtx[1]->zv,
                    .rhw = vtx[1]->rhw,
                    .xs = vtx[1]->xs,
                    .ys = vtx[1]->ys,
                    .g = vtx[1]->g,
                },
                {
                    .xv = vtx[2]->xv,
                    .yv = vtx[2]->yv,
                    .zv = vtx[2]->zv,
                    .rhw = vtx[2]->rhw,
                    .xs = vtx[2]->xs,
                    .ys = vtx[2]->ys,
                    .g = vtx[2]->g,
                },
            };

            num_points = Render_ZedClipper(num_points, points, m_VBuffer);
            if (num_points == 0) {
                continue;
            }
        }

        num_points = Render_XYGClipper(num_points, m_VBuffer);
        if (num_points == 0) {
            continue;
        }

        const float zv = Render_CalculatePolyZ(
            sort_type, vtx[0]->zv, vtx[1]->zv, vtx[2]->zv, -1.0);
        g_Sort3DPtr->_0 = g_Info3DPtr;
        g_Sort3DPtr->_1 = MAKE_ZSORT(zv);
        g_Sort3DPtr++;

        *g_Info3DPtr++ = POLY_GOURAUD;
        *g_Info3DPtr++ = face->palette_idx;
        *g_Info3DPtr++ = num_points;

        for (int32_t j = 0; j < num_points; j++) {
            *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.x;
            *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.y;
            *g_Info3DPtr++ = (int32_t)m_VBuffer[j].g;
        }
        g_SurfaceCount++;
    }
}

static void M_InsertFlatFace4s(
    RENDERER *const renderer, const FACE4 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
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

            m_VBuffer[0].pos.x = vtx[0]->xs;
            m_VBuffer[0].pos.y = vtx[0]->ys;
            m_VBuffer[0].rhw = vtx[0]->rhw;
            m_VBuffer[0].g = vtx[0]->g;

            m_VBuffer[1].pos.x = vtx[1]->xs;
            m_VBuffer[1].pos.y = vtx[1]->ys;
            m_VBuffer[1].rhw = vtx[1]->rhw;
            m_VBuffer[1].g = vtx[1]->g;

            m_VBuffer[2].pos.x = vtx[2]->xs;
            m_VBuffer[2].pos.y = vtx[2]->ys;
            m_VBuffer[2].rhw = vtx[2]->rhw;
            m_VBuffer[2].g = vtx[2]->g;

            m_VBuffer[3].pos.x = vtx[3]->xs;
            m_VBuffer[3].pos.y = vtx[3]->ys;
            m_VBuffer[3].rhw = vtx[3]->rhw;
            m_VBuffer[3].g = vtx[3]->g;
        } else {
            if (!Render_VisibleZClip(vtx[0], vtx[1], vtx[2])) {
                continue;
            }

            const POINT_INFO points[4] = {
                {
                    .xv = vtx[0]->xv,
                    .yv = vtx[0]->yv,
                    .zv = vtx[0]->zv,
                    .rhw = vtx[0]->rhw,
                    .xs = vtx[0]->xs,
                    .ys = vtx[0]->ys,
                    .g = vtx[0]->g,
                },
                {
                    .xv = vtx[1]->xv,
                    .yv = vtx[1]->yv,
                    .zv = vtx[1]->zv,
                    .rhw = vtx[1]->rhw,
                    .xs = vtx[1]->xs,
                    .ys = vtx[1]->ys,
                    .g = vtx[1]->g,
                },
                {
                    .xv = vtx[2]->xv,
                    .yv = vtx[2]->yv,
                    .zv = vtx[2]->zv,
                    .rhw = vtx[2]->rhw,
                    .xs = vtx[2]->xs,
                    .ys = vtx[2]->ys,
                    .g = vtx[2]->g,
                },
                {
                    .xv = vtx[3]->xv,
                    .yv = vtx[3]->yv,
                    .zv = vtx[3]->zv,
                    .rhw = vtx[3]->rhw,
                    .xs = vtx[3]->xs,
                    .ys = vtx[3]->ys,
                    .g = vtx[3]->g,
                },
            };

            num_points = Render_ZedClipper(num_points, points, m_VBuffer);
            if (num_points == 0) {
                continue;
            }
        }

        num_points = Render_XYGClipper(num_points, m_VBuffer);
        if (num_points == 0) {
            continue;
        }

        const float zv = Render_CalculatePolyZ(
            sort_type, vtx[0]->zv, vtx[1]->zv, vtx[2]->zv, vtx[3]->zv);
        g_Sort3DPtr->_0 = g_Info3DPtr;
        g_Sort3DPtr->_1 = MAKE_ZSORT(zv);
        g_Sort3DPtr++;

        *g_Info3DPtr++ = POLY_GOURAUD;
        *g_Info3DPtr++ = face->palette_idx;
        *g_Info3DPtr++ = num_points;

        for (int32_t j = 0; j < num_points; j++) {
            *g_Info3DPtr++ = m_VBuffer[j].pos.x;
            *g_Info3DPtr++ = m_VBuffer[j].pos.y;
            *g_Info3DPtr++ = m_VBuffer[j].g;
        }
        g_SurfaceCount++;
    }
}

static void M_InsertTexturedFace3s(
    RENDERER *const renderer, const FACE3 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    for (int32_t i = 0; i < num; i++) {
        int32_t num_points = 3;
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

        const int8_t clip_or = vtx[0]->clip | vtx[1]->clip | vtx[2]->clip;
        const int8_t clip_and = vtx[0]->clip & vtx[1]->clip & vtx[2]->clip;

        if (clip_and != 0) {
            continue;
        }

        if (clip_or >= 0) {
            if (!VBUF_VISIBLE(*vtx[0], *vtx[1], *vtx[2])) {
                continue;
            }

            if (clip_or == 0) {
                const float zv = Render_CalculatePolyZ(
                    sort_type, vtx[0]->zv, vtx[1]->zv, vtx[2]->zv, -1.0);
                g_Sort3DPtr->_0 = g_Info3DPtr;
                g_Sort3DPtr->_1 = MAKE_ZSORT(zv);
                g_Sort3DPtr++;

                if (zv >= (double)g_PerspectiveDistance) {
                    *g_Info3DPtr++ = (texture->draw_type == DRAW_OPAQUE)
                        ? POLY_GTMAP
                        : POLY_WGTMAP;
                    *g_Info3DPtr++ = texture->tex_page;
                    *g_Info3DPtr++ = 3;

                    *g_Info3DPtr++ = (int32_t)vtx[0]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[0]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[0]->g;
                    *g_Info3DPtr++ = uv[0].u;
                    *g_Info3DPtr++ = uv[0].v;

                    *g_Info3DPtr++ = (int32_t)vtx[1]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[1]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[1]->g;
                    *g_Info3DPtr++ = uv[1].u;
                    *g_Info3DPtr++ = uv[1].v;

                    *g_Info3DPtr++ = (int32_t)vtx[2]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[2]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[2]->g;
                    *g_Info3DPtr++ = uv[2].u;
                    *g_Info3DPtr++ = uv[2].v;
                } else {
                    *g_Info3DPtr++ = (texture->draw_type == DRAW_OPAQUE)
                        ? POLY_GTMAP_PERSP
                        : POLY_WGTMAP_PERSP;
                    *g_Info3DPtr++ = texture->tex_page;
                    *g_Info3DPtr++ = 3;

                    *g_Info3DPtr++ = (int32_t)vtx[0]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[0]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[0]->g;
                    *(float *)g_Info3DPtr = vtx[0]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[0].u * vtx[0]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[0].v * vtx[0]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);

                    *g_Info3DPtr++ = (int32_t)vtx[1]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[1]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[1]->g;
                    *(float *)g_Info3DPtr = vtx[1]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[1].u * vtx[1]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[1].v * vtx[1]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);

                    *g_Info3DPtr++ = (int32_t)vtx[2]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[2]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[2]->g;
                    *(float *)g_Info3DPtr = vtx[2]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[2].u * vtx[2]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[2].v * vtx[2]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                }
                g_SurfaceCount++;
                continue;
            }

            m_VBuffer[0].pos.x = vtx[0]->xs;
            m_VBuffer[0].pos.y = vtx[0]->ys;
            m_VBuffer[0].rhw = vtx[0]->rhw;
            m_VBuffer[0].g = vtx[0]->g;
            m_VBuffer[0].tex.u = (double)uv[0].u * vtx[0]->rhw;
            m_VBuffer[0].tex.v = (double)uv[0].v * vtx[0]->rhw;

            m_VBuffer[1].pos.x = vtx[1]->xs;
            m_VBuffer[1].pos.y = vtx[1]->ys;
            m_VBuffer[1].rhw = vtx[1]->rhw;
            m_VBuffer[1].g = vtx[1]->g;
            m_VBuffer[1].tex.u = (double)uv[1].u * vtx[1]->rhw;
            m_VBuffer[1].tex.v = (double)uv[1].v * vtx[1]->rhw;

            m_VBuffer[2].pos.x = vtx[2]->xs;
            m_VBuffer[2].pos.y = vtx[2]->ys;
            m_VBuffer[2].rhw = vtx[2]->rhw;
            m_VBuffer[2].g = vtx[2]->g;
            m_VBuffer[2].tex.u = (double)uv[2].u * vtx[2]->rhw;
            m_VBuffer[2].tex.v = (double)uv[2].v * vtx[2]->rhw;
        } else {
            if (!Render_VisibleZClip(vtx[0], vtx[1], vtx[2])) {
                continue;
            }

            const POINT_INFO points[3] = {
                {
                    .xv = vtx[0]->xv,
                    .yv = vtx[0]->yv,
                    .zv = vtx[0]->zv,
                    .rhw = vtx[0]->rhw,
                    .xs = vtx[0]->xs,
                    .ys = vtx[0]->ys,
                    .g = vtx[0]->g,
                    .tex = {
                        .u = uv[0].u,
                        .v = uv[0].v,
                    },
                },
                {
                    .yv = vtx[1]->yv,
                    .xv = vtx[1]->xv,
                    .zv = vtx[1]->zv,
                    .rhw = vtx[1]->rhw,
                    .xs = vtx[1]->xs,
                    .ys = vtx[1]->ys,
                    .g = vtx[1]->g,
                    .tex = {
                        .u = uv[1].u,
                        .v = uv[1].v,
                    },
                },
                {
                    .xv = vtx[2]->xv,
                    .yv = vtx[2]->yv,
                    .zv = vtx[2]->zv,
                    .rhw = vtx[2]->rhw,
                    .xs = vtx[2]->xs,
                    .ys = vtx[2]->ys,
                    .g = vtx[2]->g,
                    .tex = {
                        .u = uv[2].u,
                        .v = uv[2].v,
                    },
                },
            };

            num_points = Render_ZedClipper(num_points, points, m_VBuffer);
            if (num_points == 0) {
                continue;
            }
        }

        num_points = Render_XYGUVClipper(num_points, m_VBuffer);
        if (num_points == 0) {
            continue;
        }

        const float zv = Render_CalculatePolyZ(
            sort_type, vtx[0]->zv, vtx[1]->zv, vtx[2]->zv, -1.0);
        g_Sort3DPtr->_0 = g_Info3DPtr;
        g_Sort3DPtr->_1 = MAKE_ZSORT(zv);
        g_Sort3DPtr++;

        if (zv >= (double)g_PerspectiveDistance) {
            *g_Info3DPtr++ =
                (texture->draw_type == DRAW_OPAQUE) ? POLY_GTMAP : POLY_WGTMAP;
            *g_Info3DPtr++ = texture->tex_page;
            *g_Info3DPtr++ = num_points;

            for (int32_t j = 0; j < num_points; j++) {
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.x;
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.y;
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].g;
                *g_Info3DPtr++ =
                    (int32_t)(m_VBuffer[j].tex.u / m_VBuffer[j].rhw);
                *g_Info3DPtr++ =
                    (int32_t)(m_VBuffer[j].tex.v / m_VBuffer[j].rhw);
            }
        } else {
            *g_Info3DPtr++ = (texture->draw_type == DRAW_OPAQUE)
                ? POLY_GTMAP_PERSP
                : POLY_WGTMAP_PERSP;
            *g_Info3DPtr++ = texture->tex_page;
            *g_Info3DPtr++ = num_points;

            for (int32_t j = 0; j < num_points; j++) {
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.x;
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.y;
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].g;
                *(float *)g_Info3DPtr = m_VBuffer[j].rhw;
                g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                *(float *)g_Info3DPtr = m_VBuffer[j].tex.u;
                g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                *(float *)g_Info3DPtr = m_VBuffer[j].tex.v;
                g_Info3DPtr += sizeof(float) / sizeof(int16_t);
            }
        }
        g_SurfaceCount++;
    }
}

static void M_InsertTexturedFace4s(
    RENDERER *const renderer, const FACE4 *const faces, const int32_t num,
    const SORT_TYPE sort_type)
{
    for (int32_t i = 0; i < num; i++) {
        int32_t num_points = 4;
        const FACE4 *const face = &faces[i];
        const PHD_VBUF *const vtx[4] = {
            &g_PhdVBuf[face->vertices[0]],
            &g_PhdVBuf[face->vertices[1]],
            &g_PhdVBuf[face->vertices[2]],
            &g_PhdVBuf[face->vertices[3]],
        };

        const OBJECT_TEXTURE *const texture =
            Output_GetObjectTexture(face->texture_idx);
        const TEXTURE_UV *const uv = texture->uv;

        if (texture->draw_type != DRAW_OPAQUE && g_DiscardTransparent) {
            continue;
        }

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

            if (clip_or == 0) {
                const float zv = Render_CalculatePolyZ(
                    sort_type, vtx[0]->zv, vtx[1]->zv, vtx[2]->zv, vtx[3]->zv);
                g_Sort3DPtr->_0 = g_Info3DPtr;
                g_Sort3DPtr->_1 = MAKE_ZSORT(zv);
                g_Sort3DPtr++;

                if (zv >= (double)g_PerspectiveDistance) {
                    *g_Info3DPtr++ = (texture->draw_type == DRAW_OPAQUE)
                        ? POLY_GTMAP
                        : POLY_WGTMAP;
                    *g_Info3DPtr++ = texture->tex_page;
                    *g_Info3DPtr++ = 4;

                    *g_Info3DPtr++ = (int32_t)vtx[0]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[0]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[0]->g;
                    *g_Info3DPtr++ = uv[0].u;
                    *g_Info3DPtr++ = uv[0].v;

                    *g_Info3DPtr++ = (int32_t)vtx[1]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[1]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[1]->g;
                    *g_Info3DPtr++ = uv[1].u;
                    *g_Info3DPtr++ = uv[1].v;

                    *g_Info3DPtr++ = (int32_t)vtx[2]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[2]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[2]->g;
                    *g_Info3DPtr++ = uv[2].u;
                    *g_Info3DPtr++ = uv[2].v;

                    *g_Info3DPtr++ = (int32_t)vtx[3]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[3]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[3]->g;
                    *g_Info3DPtr++ = uv[3].u;
                    *g_Info3DPtr++ = uv[3].v;
                } else {
                    *g_Info3DPtr++ = (texture->draw_type == DRAW_OPAQUE)
                        ? POLY_GTMAP_PERSP
                        : POLY_WGTMAP_PERSP;
                    *g_Info3DPtr++ = texture->tex_page;
                    *g_Info3DPtr++ = 4;

                    *g_Info3DPtr++ = (int32_t)vtx[0]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[0]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[0]->g;
                    *(float *)g_Info3DPtr = vtx[0]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[0].u * vtx[0]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[0].v * vtx[0]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);

                    *g_Info3DPtr++ = (int32_t)vtx[1]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[1]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[1]->g;
                    *(float *)g_Info3DPtr = vtx[1]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[1].u * vtx[1]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[1].v * vtx[1]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);

                    *g_Info3DPtr++ = (int32_t)vtx[2]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[2]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[2]->g;
                    *(float *)g_Info3DPtr = vtx[2]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[2].u * vtx[2]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[2].v * vtx[2]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);

                    *g_Info3DPtr++ = (int32_t)vtx[3]->xs;
                    *g_Info3DPtr++ = (int32_t)vtx[3]->ys;
                    *g_Info3DPtr++ = (int32_t)vtx[3]->g;
                    *(float *)g_Info3DPtr = vtx[3]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[3].u * vtx[3]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                    *(float *)g_Info3DPtr = (double)uv[3].v * vtx[3]->rhw;
                    g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                }
                g_SurfaceCount++;
                continue;
            }

            m_VBuffer[0].pos.x = vtx[0]->xs;
            m_VBuffer[0].pos.y = vtx[0]->ys;
            m_VBuffer[0].rhw = vtx[0]->rhw;
            m_VBuffer[0].g = vtx[0]->g;
            m_VBuffer[0].tex.u = (double)uv[0].u * vtx[0]->rhw;
            m_VBuffer[0].tex.v = (double)uv[0].v * vtx[0]->rhw;

            m_VBuffer[1].pos.x = vtx[1]->xs;
            m_VBuffer[1].pos.y = vtx[1]->ys;
            m_VBuffer[1].rhw = vtx[1]->rhw;
            m_VBuffer[1].g = vtx[1]->g;
            m_VBuffer[1].tex.u = (double)uv[1].u * vtx[1]->rhw;
            m_VBuffer[1].tex.v = (double)uv[1].v * vtx[1]->rhw;

            m_VBuffer[2].pos.x = vtx[2]->xs;
            m_VBuffer[2].pos.y = vtx[2]->ys;
            m_VBuffer[2].rhw = vtx[2]->rhw;
            m_VBuffer[2].g = vtx[2]->g;
            m_VBuffer[2].tex.u = (double)uv[2].u * vtx[2]->rhw;
            m_VBuffer[2].tex.v = (double)uv[2].v * vtx[2]->rhw;

            m_VBuffer[3].pos.x = vtx[3]->xs;
            m_VBuffer[3].pos.y = vtx[3]->ys;
            m_VBuffer[3].rhw = vtx[3]->rhw;
            m_VBuffer[3].g = vtx[3]->g;
            m_VBuffer[3].tex.u = (double)uv[3].u * vtx[3]->rhw;
            m_VBuffer[3].tex.v = (double)uv[3].v * vtx[3]->rhw;
        } else {
            if (!Render_VisibleZClip(vtx[0], vtx[1], vtx[2])) {
                continue;
            }

            const POINT_INFO points[4] = {
                {
                    .xv = vtx[0]->xv,
                    .yv = vtx[0]->yv,
                    .zv = vtx[0]->zv,
                    .rhw = vtx[0]->rhw,
                    .xs = vtx[0]->xs,
                    .ys = vtx[0]->ys,
                    .g = vtx[0]->g,
                    .tex = {
                        .u = uv[0].u,
                        .v = uv[0].v,
                    },
                },
                {
                    .yv = vtx[1]->yv,
                    .xv = vtx[1]->xv,
                    .zv = vtx[1]->zv,
                    .rhw = vtx[1]->rhw,
                    .xs = vtx[1]->xs,
                    .ys = vtx[1]->ys,
                    .g = vtx[1]->g,
                    .tex = {
                        .u = uv[1].u,
                        .v = uv[1].v,
                    },
                },
                {
                    .xv = vtx[2]->xv,
                    .yv = vtx[2]->yv,
                    .zv = vtx[2]->zv,
                    .rhw = vtx[2]->rhw,
                    .xs = vtx[2]->xs,
                    .ys = vtx[2]->ys,
                    .g = vtx[2]->g,
                    .tex = {
                        .u = uv[2].u,
                        .v = uv[2].v,
                    },
                },
                {
                    .xv = vtx[3]->xv,
                    .yv = vtx[3]->yv,
                    .zv = vtx[3]->zv,
                    .rhw = vtx[3]->rhw,
                    .xs = vtx[3]->xs,
                    .ys = vtx[3]->ys,
                    .g = vtx[3]->g,
                    .tex = {
                        .u = uv[3].u,
                        .v = uv[3].v,
                    },
                },
            };

            num_points = Render_ZedClipper(num_points, points, m_VBuffer);
            if (num_points == 0) {
                continue;
            }
        }

        num_points = Render_XYGUVClipper(num_points, m_VBuffer);
        if (num_points == 0) {
            continue;
        }

        const float zv = Render_CalculatePolyZ(
            sort_type, vtx[0]->zv, vtx[1]->zv, vtx[2]->zv, vtx[3]->zv);
        g_Sort3DPtr->_0 = g_Info3DPtr;
        g_Sort3DPtr->_1 = MAKE_ZSORT(zv);
        g_Sort3DPtr++;

        if (zv >= (double)g_PerspectiveDistance) {
            *g_Info3DPtr++ =
                (texture->draw_type == DRAW_OPAQUE) ? POLY_GTMAP : POLY_WGTMAP;
            *g_Info3DPtr++ = texture->tex_page;
            *g_Info3DPtr++ = num_points;

            for (int32_t j = 0; j < num_points; j++) {
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.x;
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.y;
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].g;
                *g_Info3DPtr++ =
                    (int32_t)(m_VBuffer[j].tex.u / m_VBuffer[j].rhw);
                *g_Info3DPtr++ =
                    (int32_t)(m_VBuffer[j].tex.v / m_VBuffer[j].rhw);
            }
        } else {
            *g_Info3DPtr++ = (texture->draw_type == DRAW_OPAQUE)
                ? POLY_GTMAP_PERSP
                : POLY_WGTMAP_PERSP;
            *g_Info3DPtr++ = texture->tex_page;
            *g_Info3DPtr++ = num_points;

            for (int32_t j = 0; j < num_points; j++) {
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.x;
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].pos.y;
                *g_Info3DPtr++ = (int32_t)m_VBuffer[j].g;
                *(float *)g_Info3DPtr = m_VBuffer[j].rhw;
                g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                *(float *)g_Info3DPtr = m_VBuffer[j].tex.u;
                g_Info3DPtr += sizeof(float) / sizeof(int16_t);
                *(float *)g_Info3DPtr = m_VBuffer[j].tex.v;
                g_Info3DPtr += sizeof(float) / sizeof(int16_t);
            }
        }
        g_SurfaceCount++;
    }
}

static void M_InsertLine(
    RENDERER *const renderer, const int32_t x1, const int32_t y1,
    const int32_t x2, const int32_t y2, const int32_t z,
    const uint8_t color_idx)
{
    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = POLY_LINE;
    *g_Info3DPtr++ = x1;
    *g_Info3DPtr++ = y1;
    *g_Info3DPtr++ = x2;
    *g_Info3DPtr++ = y2;
    *g_Info3DPtr++ = color_idx;

    g_SurfaceCount++;
}

static void M_InsertFlatRect(
    RENDERER *const renderer, const int32_t x1, const int32_t y1,
    const int32_t x2, const int32_t y2, int32_t z, uint8_t color_idx)
{
    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = POLY_FLAT;
    *g_Info3DPtr++ = color_idx;
    *g_Info3DPtr++ = 4;
    *g_Info3DPtr++ = x1;
    *g_Info3DPtr++ = y1;
    *g_Info3DPtr++ = x2;
    *g_Info3DPtr++ = y1;
    *g_Info3DPtr++ = x2;
    *g_Info3DPtr++ = y2;
    *g_Info3DPtr++ = x1;
    *g_Info3DPtr++ = y2;

    g_SurfaceCount++;
}

static void M_InsertTransOctagon(
    RENDERER *const renderer, const PHD_VBUF *vbuf, const int16_t shade)
{
    const int32_t vtx_count = 8;

    int8_t clip_or = 0;
    uint8_t clip_and = 0xFF;
    for (int32_t i = 0; i < vtx_count; i++) {
        clip_or |= vbuf[i].clip;
        clip_and &= vbuf[i].clip;
    }

    if (clip_or < 0 || clip_and || !VBUF_VISIBLE(vbuf[0], vbuf[1], vbuf[2])) {
        return;
    }

    int32_t num_points = vtx_count;
    for (int32_t i = 0; i < num_points; i++) {
        m_VBuffer[i].pos.x = vbuf[i].xs;
        m_VBuffer[i].pos.y = vbuf[i].ys;
    }

    if (clip_or != 0) {
        g_FltWinTop = 0.0f;
        g_FltWinLeft = 0.0f;
        g_FltWinRight = g_PhdWinMaxX;
        g_FltWinBottom = g_PhdWinMaxY;

        num_points = Render_XYClipper(vtx_count, m_VBuffer);
        if (num_points == 0) {
            return;
        }
    }

    double poly_z = 0.0;
    for (int32_t i = 0; i < vtx_count; i++) {
        poly_z += vbuf[i].zv;
    }
    poly_z /= vtx_count;

    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(poly_z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = POLY_TRANS;
    *g_Info3DPtr++ = shade;
    *g_Info3DPtr++ = num_points;
    for (int32_t i = 0; i < num_points; i++) {
        *g_Info3DPtr++ = m_VBuffer[i].pos.x;
        *g_Info3DPtr++ = m_VBuffer[i].pos.y;
    }
    g_SurfaceCount++;
}

static void M_InsertTransQuad(
    RENDERER *const renderer, const int32_t x, const int32_t y,
    const int32_t width, int32_t height, const int32_t z)
{
    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(g_PhdNearZ + 8 * z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = POLY_TRANS;
    *g_Info3DPtr++ = 24;
    *g_Info3DPtr++ = 4; // number of vertices
    *g_Info3DPtr++ = x;
    *g_Info3DPtr++ = y;
    *g_Info3DPtr++ = x + width;
    *g_Info3DPtr++ = y;
    *g_Info3DPtr++ = x + width;
    *g_Info3DPtr++ = height + y;
    *g_Info3DPtr++ = x;
    *g_Info3DPtr++ = height + y;

    g_SurfaceCount++;
}

static void M_InsertSprite(
    RENDERER *const renderer, const int32_t z, const int32_t x0,
    const int32_t y0, const int32_t x1, const int32_t y1,
    const int32_t sprite_idx, const int16_t shade)
{
    g_Sort3DPtr->_0 = g_Info3DPtr;
    g_Sort3DPtr->_1 = MAKE_ZSORT(z);
    g_Sort3DPtr++;

    *g_Info3DPtr++ = POLY_SPRITE;
    *g_Info3DPtr++ = x0;
    *g_Info3DPtr++ = y0;
    *g_Info3DPtr++ = x1;
    *g_Info3DPtr++ = y1;
    *g_Info3DPtr++ = sprite_idx;
    *g_Info3DPtr++ = shade;
    g_SurfaceCount++;
}

void Renderer_SW_Prepare(RENDERER *const renderer)
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
    renderer->ResetPolyList = nullptr;
    renderer->EnableZBuffer = nullptr;
    renderer->ClearZBuffer = nullptr;
    renderer->SetWet = M_SetWet;

    renderer->InsertFlatFace3s = M_InsertFlatFace3s;
    renderer->InsertFlatFace4s = M_InsertFlatFace4s;
    renderer->InsertTexturedFace3s = M_InsertTexturedFace3s;
    renderer->InsertTexturedFace4s = M_InsertTexturedFace4s;
    renderer->InsertLine = M_InsertLine;
    renderer->InsertFlatRect = M_InsertFlatRect;
    renderer->InsertTransQuad = M_InsertTransQuad;
    renderer->InsertTransOctagon = M_InsertTransOctagon;
    renderer->InsertSprite = M_InsertSprite;
}
