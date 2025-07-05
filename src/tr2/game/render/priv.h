#pragma once

#include "game/output.h"
#include "game/render/common.h"

#define VBUF_VISIBLE(a, b, c)                                                  \
    (((a).ys - (b).ys) * ((c).xs - (b).xs)                                     \
     >= ((c).ys - (b).ys) * ((a).xs - (b).xs))
#define MAKE_ZSORT(z) ((uint32_t)(z))

typedef struct RENDERER {
    void (*Init)(struct RENDERER *);
    void (*Shutdown)(struct RENDERER *);

    void (*Open)(struct RENDERER *);
    void (*Close)(struct RENDERER *);

    void (*Reset)(struct RENDERER *, RENDER_RESET_FLAGS source);

    void (*BeginScene)(struct RENDERER *);
    void (*EndScene)(struct RENDERER *);

    void (*ResetPolyList)(struct RENDERER *);
    void (*ClearZBuffer)(struct RENDERER *);
    void (*EnableZBuffer)(struct RENDERER *, bool, bool);
    void (*DrawPolyList)(struct RENDERER *);
    void (*SetWet)(struct RENDERER *, bool);
    void (*SwitchViewport)(struct RENDERER *, VIEWPORT_SPACE);

    const int16_t *(*InsertGT4)(
        struct RENDERER *renderer, const int16_t *obj_ptr, int32_t num,
        SORT_TYPE sort_type);

    void (*InsertFlatFace3s)(
        struct RENDERER *renderer, const FACE3 *faces, int32_t num,
        SORT_TYPE sort_type);
    void (*InsertFlatFace4s)(
        struct RENDERER *renderer, const FACE4 *faces, int32_t num,
        SORT_TYPE sort_type);
    void (*InsertTexturedFace3s)(
        struct RENDERER *renderer, const FACE3 *faces, int32_t num,
        SORT_TYPE sort_type);
    void (*InsertTexturedFace4s)(
        struct RENDERER *renderer, const FACE4 *faces, int32_t num,
        SORT_TYPE sort_type);
    void (*InsertTransQuad)(
        struct RENDERER *renderer, int32_t x, int32_t y, int32_t width,
        int32_t height, int32_t z, uint8_t alpha);
    void (*InsertTransOctagon)(
        struct RENDERER *renderer, const PHD_VBUF *vbuf, int16_t shade);
    void (*InsertFlatRect)(
        struct RENDERER *renderer, int32_t x0, int32_t y0, int32_t x1,
        int32_t y1, int32_t z, uint8_t color_idx);
    void (*InsertSprite)(
        struct RENDERER *renderer, int32_t z, int32_t x0, int32_t y0,
        int32_t x1, int32_t y1, int32_t sprite_idx, int16_t shade);
    void (*InsertLine)(
        struct RENDERER *renderer, int32_t x0, int32_t y0, int32_t x1,
        int32_t y1, int32_t z, uint8_t color_idx);

    bool initialized;
    bool open;

    void *priv;
} RENDERER;

// TODO: don't be a global
extern bool g_DiscardTransparent;

double Render_CalculatePolyZ(
    SORT_TYPE sort_type, double z0, double z1, double z2, double z3);

void Render_SortPolyList(void);
int32_t Render_GetUVAdjustment(void);
void Render_ResetTextureUVs(void);
void Render_AdjustTextureUVs(bool reset_uv_add);

int32_t Render_VisibleZClip(
    const PHD_VBUF *vtx0, const PHD_VBUF *vtx1, const PHD_VBUF *vtx2);
int32_t Render_ZedClipper(
    int32_t vtx_count, const POINT_INFO *pts, VERTEX_INFO *vtx);
int32_t Render_XYClipper(int32_t vtx_count, VERTEX_INFO *vtx);
int32_t Render_XYGClipper(int32_t vtx_count, VERTEX_INFO *vtx);
int32_t Render_XYGUVClipper(int32_t vtx_count, VERTEX_INFO *vtx);
