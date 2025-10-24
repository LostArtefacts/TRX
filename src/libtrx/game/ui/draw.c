#include "game/ui/draw.h"

#include "game/objects.h"
#include "game/output/common.h"
#include "game/output/sources/ui.h"
#include "game/output/state.h"
#include "game/scaler.h"
#include "memory.h"
#include "vector.h"
#include "version.h"

#include <string.h>

#define M_OUTLINE_THICKNESS 0.75f

typedef enum {
    C_BACKGROUND_E,
    C_BACKGROUND_C,
    C_BACKGROUND_HEAVY_E,
    C_BACKGROUND_HEAVY_C,
    C_HEADING_E,
    C_HEADING_C,
    C_REQUESTED_E,
    C_REQUESTED_C,
    C_REQUESTED_OUTLINE_CH,
    C_REQUESTED_OUTLINE_CV,
    C_REQUESTED_OUTLINE_E,
    C_BACKGROUND_OUTLINE_TL,
    C_BACKGROUND_OUTLINE_TR,
    C_BACKGROUND_OUTLINE_BL,
    C_BACKGROUND_OUTLINE_BR,
    C_HEADING_OUTLINE,
    C_GENERIC_OUTLINE_LIGHT,
    C_GENERIC_OUTLINE_DARK,
    C_NUMBER_OF,
} M_COLOR;

typedef union {
    M_COLOR colors[2];
    struct {
        M_COLOR edge;
        M_COLOR center;
    };
} M_GRADIENT_FILL;

static const M_GRADIENT_FILL m_GradientFills[] = {
    // clang-format off
    [TS_BACKGROUND]       = { .edge = C_BACKGROUND_E, .center = C_BACKGROUND_C },
    [TS_BACKGROUND_HEAVY] = { .edge = C_BACKGROUND_HEAVY_E, .center = C_BACKGROUND_HEAVY_C },
    [TS_HEADING]          = { .edge = C_HEADING_E, .center = C_HEADING_C },
    [TS_REQUESTED]        = { .edge = C_REQUESTED_E, .center = C_REQUESTED_C },
    // clang-format on
};

static const RGBA_8888 m_MenuColorMap[TR_VERSION_COUNT][C_NUMBER_OF] = {
    // clang-format off
    [0] = {
        [C_BACKGROUND_C]           = { 0x00, 0x00, 0x40, 0x80 },
        [C_BACKGROUND_E]           = { 0x00, 0x00, 0x00, 0x80 },
        [C_BACKGROUND_HEAVY_C]     = { 0x00, 0x00, 0x00, 0xE0 },
        [C_BACKGROUND_HEAVY_E]     = { 0x00, 0x00, 0x00, 0xE0 },
        [C_HEADING_E]              = { 0x00, 0x00, 0x00, 0x80 },
        [C_HEADING_C]              = { 0x80, 0x38, 0x10, 0x80 },
        [C_REQUESTED_E]            = { 0x00, 0x00, 0x00, 0x80 },
        [C_REQUESTED_C]            = { 0x80, 0x38, 0xDC, 0x80 },
        //[C_REQUESTED_C]            = { 0x40, 0x1C, 0x78, 0x80 },
        [C_REQUESTED_OUTLINE_CH]   = { 0xC8, 0xC8, 0xC8, 0xFF },
        [C_REQUESTED_OUTLINE_CV]   = { 0xC8, 0xC8, 0xC8, 0xFF },
        [C_REQUESTED_OUTLINE_E]    = { 0x28, 0x28, 0x28, 0xFF },
        [C_BACKGROUND_OUTLINE_TL]  = { 0x60, 0x60, 0x60, 0xFF },
        [C_BACKGROUND_OUTLINE_TR]  = { 0x20, 0x20, 0x20, 0xFF },
        [C_BACKGROUND_OUTLINE_BL]  = { 0x40, 0x40, 0x40, 0xFF },
        [C_BACKGROUND_OUTLINE_BR]  = { 0x00, 0x00, 0x00, 0xFF },
        [C_HEADING_OUTLINE]        = { 0x00, 0x00, 0x00, 0xFF },
        [C_GENERIC_OUTLINE_LIGHT]  = { 0xE8, 0xC0, 0x70, 0xFF },
        [C_GENERIC_OUTLINE_DARK]   = { 0x8C, 0x70, 0x38, 0xFF },
    },
    [1] = {
        [C_BACKGROUND_E]           = { 0x00, 0x20, 0x00, 0x80 },
        [C_BACKGROUND_C]           = { 0x00, 0x60, 0x00, 0x80 },
        [C_BACKGROUND_HEAVY_E]     = { 0x00, 0x00, 0x00, 0xE0 },
        [C_BACKGROUND_HEAVY_C]     = { 0x00, 0x20, 0x00, 0xE0 },
        [C_HEADING_E]              = { 0x00, 0x00, 0x00, 0x80 },
        [C_HEADING_C]              = { 0x10, 0x80, 0x38, 0x80 },
        [C_REQUESTED_E]            = { 0x00, 0x00, 0x00, 0x80 },
        [C_REQUESTED_C]            = { 0x38, 0xF0, 0x80, 0x80 },
        [C_REQUESTED_OUTLINE_CH]   = { 0xFF, 0xFF, 0xFF, 0xFF },
        [C_REQUESTED_OUTLINE_CV]   = { 0x38, 0xF0, 0x80, 0xFF },
        [C_REQUESTED_OUTLINE_E]    = { 0x00, 0x00, 0x00, 0xFF },
        [C_BACKGROUND_OUTLINE_TL]  = { 0x60, 0x60, 0x60, 0xFF },
        [C_BACKGROUND_OUTLINE_TR]  = { 0x20, 0x20, 0x20, 0xFF },
        [C_BACKGROUND_OUTLINE_BL]  = { 0x40, 0x40, 0x40, 0xFF },
        [C_BACKGROUND_OUTLINE_BR]  = { 0x00, 0x00, 0x00, 0xFF },
        [C_HEADING_OUTLINE]        = { 0x00, 0x00, 0x00, 0xFF },
        [C_GENERIC_OUTLINE_LIGHT]  = { 0xE8, 0xC0, 0x70, 0xFF },
        [C_GENERIC_OUTLINE_DARK]   = { 0x8C, 0x70, 0x38, 0xFF },
    },
    // clang-format on
};

// Draw operation types for deferred UI rendering.
typedef enum {
    UI_DRAW_OP_TEXT_BACKGROUND,
    UI_DRAW_OP_TEXT_OUTLINE,
    UI_DRAW_OP_SCREEN_SPRITE,
    UI_DRAW_OP_FLAT_QUAD,
    UI_DRAW_OP_GRADIENT_QUAD,
    UI_DRAW_OP_HORZ_LINE,
    UI_DRAW_OP_FADER_DRAW,
} M_DRAW_OP_TYPE;

// Deferred draw operation node.
typedef struct {
    M_DRAW_OP_TYPE type;
    union {
        struct {
            UI_STYLE ui_style;
            int32_t x0, x1, y, z;
        } horz_line;
        struct {
            UI_STYLE ui_style;
            int32_t x0, y0, x1, y1, z;
            TEXT_STYLE text_style;
        } text;
        struct {
            int32_t sx, sy, z, scale_h, scale_v, sprite_idx;
            int16_t shade;
        } sprite;
        struct {
            FADER *fader;
        } fader;
        struct {
            int32_t sx, sy, z, w, h;
            RGBA_8888 color;
        } flat_quad;
        struct {
            int32_t sx, sy, z, w, h;
            RGBA_8888 tl, tr, bl, br;
        } gradient_quad;
    } data;
} M_DRAW_OP;

typedef struct {
    MEMORY_ARENA_ALLOCATOR alloc;
    VECTOR *ops;
} M_PRIV;

static M_PRIV m_Priv = {
    .alloc = {
        .default_chunk_size = 1024 * 4,
    },
};

static RGBA_8888 M_GetMenuColor(const M_COLOR color)
{
    return m_MenuColorMap[g_TRVersion - 1][color];
}

static void M_DrawScreenQuad(
    const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1,
    const int32_t z, const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br)
{
    OutputSource_UI_StageQuad((OUTPUT_UI_QUAD) {
        .x0 = x0,
        .y0 = y0,
        .x1 = x1,
        .y1 = y1,
        .tl = tl,
        .tr = tr,
        .bl = bl,
        .br = br,
        .z = Output_GetNearZ_UI() + z,
    });
}

static void M_DrawScreenSprite(
    const int32_t sx, const int32_t sy, const int32_t sz, const int32_t scale_h,
    const int32_t scale_v, const int32_t sprite_idx, const int16_t shade)
{
    Output_DrawScreenSprite(sx, sy, sz, scale_h, scale_v, sprite_idx, shade);
}

static void M_DrawScreenGradientBox(
    const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1,
    const int32_t z, const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br, const float thickness)
{
    const float e = Scaler_Calc(thickness, SCALER_TARGET_TEXT);
    M_DrawScreenQuad(x0 - e, y0 - e, x1 + e, y0 + e, z, tl, tr, tl, tr);
    M_DrawScreenQuad(x0 - e, y1 - e, x1 + e, y1 + e, z, bl, br, bl, br);
    M_DrawScreenQuad(x0 - e, y0 - e, x0 + e, y1 + e, z, tl, tl, bl, bl);
    M_DrawScreenQuad(x1 - e, y0 - e, x1 + e, y1 + e, z, tr, tr, br, br);
}

static void M_DrawTextBackground(
    const UI_STYLE ui_style, const int32_t x0, const int32_t y0,
    const int32_t x1, const int32_t y1, const int32_t z,
    const TEXT_STYLE text_style)
{
    if (ui_style == UI_STYLE_PC) {
        const uint8_t a = text_style == TS_BACKGROUND_HEAVY ? 224 : 128;
        const RGBA_8888 cb = { 0, 0, 0, a };
        M_DrawScreenQuad(x0, y0, x1, y1, z, cb, cb, cb, cb);
        return;
    }

    const int32_t xm = (x0 + x1) / 2;
    const int32_t ym = (y0 + y1) / 2;
    const M_GRADIENT_FILL *const fill = &m_GradientFills[text_style];
#define L_DRAW(x0, y0, x1, y1, tl, tr, bl, br)                                 \
    M_DrawScreenQuad(                                                          \
        x0, y0, x1, y1, z, M_GetMenuColor(fill->colors[tl]),                   \
        M_GetMenuColor(fill->colors[tr]), M_GetMenuColor(fill->colors[bl]),    \
        M_GetMenuColor(fill->colors[br]));
    L_DRAW(xm, y0, x0, ym, 0, 0, 1, 0);
    L_DRAW(x1, y0, xm, ym, 0, 0, 0, 1);
    L_DRAW(xm, ym, x0, y1, 1, 0, 0, 0);
    L_DRAW(x1, ym, xm, y1, 0, 1, 0, 0);
#undef L_DRAW
}

static void M_DrawScreenCentreGradientBox(
    const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1,
    const int32_t z, const RGBA_8888 edge, const RGBA_8888 center_h,
    const RGBA_8888 center_v, const float thickness)
{
    const float e = Scaler_Calc(thickness, SCALER_TARGET_TEXT);
    const int32_t xm = (x0 + x1) / 2;
    const int32_t ym = (y0 + y1) / 2;
    const RGBA_8888 ch = center_h;
    const RGBA_8888 cv = center_v;
    const RGBA_8888 ce = edge;

    // clang-format off
    M_DrawScreenQuad(x0 - e, y0 - e, xm,     y0 + e, z, ce, ch, ce, ch);
    M_DrawScreenQuad(xm,     y0 - e, x1 + e, y0 + e, z, ch, ce, ch, ce);
    M_DrawScreenQuad(x0 - e, y1 - e, xm,     y1 + e, z, ce, ch, ce, ch);
    M_DrawScreenQuad(xm,     y1 - e, x1 + e, y1 + e, z, ch, ce, ch, ce);
    M_DrawScreenQuad(x0 - e, y0,     x0 + e, ym,     z, ce, ce, cv, cv);
    M_DrawScreenQuad(x0 - e, ym,     x0 + e, y1,     z, cv, cv, ce, ce);
    M_DrawScreenQuad(x1 - e, y0,     x1 + e, ym,     z, ce, ce, cv, cv);
    M_DrawScreenQuad(x1 - e, ym,     x1 + e, y1,     z, cv, cv, ce, ce);
    // clang-format on
}

static void M_DrawHorizontalLine(const M_DRAW_OP *const op)
{
    const UI_STYLE ui_style = op->data.horz_line.ui_style;
    const int32_t x0 = op->data.horz_line.x0;
    const int32_t x1 = op->data.horz_line.x1;
    const int32_t y = op->data.horz_line.y;
    const int32_t z = op->data.horz_line.z;
    const float thickness = M_OUTLINE_THICKNESS;
    const float e = Scaler_Calc(thickness, SCALER_TARGET_TEXT);
    if (g_TRVersion == 1 && ui_style == UI_STYLE_PC) {
        const RGBA_8888 cd = M_GetMenuColor(C_GENERIC_OUTLINE_DARK);
        const RGBA_8888 cl = M_GetMenuColor(C_GENERIC_OUTLINE_LIGHT);
        M_DrawScreenQuad(x0, y - e, x1, y, 0, cl, cl, cl, cl);
        M_DrawScreenQuad(x0, y, x1, y + e, 0, cd, cd, cd, cd);
    } else if (g_TRVersion == 2 && ui_style == UI_STYLE_PC) {
        const int32_t mesh_idx = Object_Get(O_TEXT_BOX)->mesh_idx;
        M_DrawScreenSprite(
            x0, y, z, (x1 - x0) * PHD_ONE / 8, PHD_ONE, mesh_idx + 4,
            SHADE_NEUTRAL);
    } else {
        const float e = Scaler_Calc(M_OUTLINE_THICKNESS, SCALER_TARGET_TEXT);
        M_DrawScreenQuad(
            x0, y - e, x1, y + e, z, M_GetMenuColor(C_BACKGROUND_OUTLINE_BL),
            M_GetMenuColor(C_BACKGROUND_OUTLINE_BR),
            M_GetMenuColor(C_BACKGROUND_OUTLINE_BL),
            M_GetMenuColor(C_BACKGROUND_OUTLINE_BR));
    }
}

static void M_DrawTextOutline(
    const UI_STYLE ui_style, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
    const int32_t z, const TEXT_STYLE text_style)
{
    if (g_TRVersion == 2 && ui_style == UI_STYLE_PC) {
        const int32_t mesh_idx = Object_Get(O_TEXT_BOX)->mesh_idx;

        const int32_t offset = text_style == 4;
        x0 += offset;
        y0 += offset;
        x1 -= offset;
        y1 -= offset;
        const int32_t scale_h = PHD_ONE;
        const int32_t scale_v = PHD_ONE;
        const int32_t w = (x1 - x0) * PHD_ONE / 8;
        const int32_t h = (y1 - y0) * PHD_ONE / 8;

        // Corners
        M_DrawScreenSprite(
            x0, y0, z, scale_h, scale_v, mesh_idx + 0, SHADE_NEUTRAL);
        M_DrawScreenSprite(
            x1, y0, z, scale_h, scale_v, mesh_idx + 1, SHADE_NEUTRAL);
        M_DrawScreenSprite(
            x1, y1, z, scale_h, scale_v, mesh_idx + 2, SHADE_NEUTRAL);
        M_DrawScreenSprite(
            x0, y1, z, scale_h, scale_v, mesh_idx + 3, SHADE_NEUTRAL);

        // Lines
        M_DrawScreenSprite(x0, y0, z, w, scale_v, mesh_idx + 4, SHADE_NEUTRAL);
        M_DrawScreenSprite(x1, y0, z, scale_h, h, mesh_idx + 5, SHADE_NEUTRAL);
        M_DrawScreenSprite(x0, y1, z, w, scale_v, mesh_idx + 6, SHADE_NEUTRAL);
        M_DrawScreenSprite(x0, y0, z, scale_h, h, mesh_idx + 7, SHADE_NEUTRAL);
        return;
    }

    if (g_TRVersion == 1 && ui_style == UI_STYLE_PC) {
        const RGBA_8888 cd = M_GetMenuColor(C_GENERIC_OUTLINE_DARK);
        const RGBA_8888 cl = M_GetMenuColor(C_GENERIC_OUTLINE_LIGHT);
        const float thickness = M_OUTLINE_THICKNESS;
        Output_DrawScreenFrame(x0, y0, x1 - x0, y1 - y0, cd, cl, thickness);
        return;
    }

    if (text_style == TS_HEADING) {
        M_DrawScreenGradientBox(
            x0, y0, x1, y1, z, M_GetMenuColor(C_HEADING_OUTLINE),
            M_GetMenuColor(C_HEADING_OUTLINE),
            M_GetMenuColor(C_HEADING_OUTLINE),
            M_GetMenuColor(C_HEADING_OUTLINE), M_OUTLINE_THICKNESS);
    } else if (
        text_style == TS_BACKGROUND || text_style == TS_BACKGROUND_HEAVY) {
        M_DrawScreenGradientBox(
            x0, y0, x1, y1, z, M_GetMenuColor(C_BACKGROUND_OUTLINE_TL),
            M_GetMenuColor(C_BACKGROUND_OUTLINE_TR),
            M_GetMenuColor(C_BACKGROUND_OUTLINE_BL),
            M_GetMenuColor(C_BACKGROUND_OUTLINE_BR), M_OUTLINE_THICKNESS);
    } else if (text_style == TS_REQUESTED) {
        M_DrawScreenCentreGradientBox(
            x0, y0, x1, y1, z, M_GetMenuColor(C_REQUESTED_OUTLINE_E),
            M_GetMenuColor(C_REQUESTED_OUTLINE_CH),
            M_GetMenuColor(C_REQUESTED_OUTLINE_CV), M_OUTLINE_THICKNESS);
    }
}

// Allocate a new deferred draw operation in the arena.
static M_DRAW_OP *M_AllocDrawOp(void)
{
    M_DRAW_OP *const op = Memory_ArenaAlloc(&m_Priv.alloc, sizeof(*op));
    memset(op, 0, sizeof(*op));
    return op;
}

static void M_ScheduleOp(M_DRAW_OP *const op)
{
    M_PRIV *const p = &m_Priv;
    Vector_Add(p->ops, &op);
}

void UI_ScheduleDrawTextBackground(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, const int32_t w, const int32_t h,
    const TEXT_STYLE text_style)
{
    M_DRAW_OP *const op = M_AllocDrawOp();
    op->type = UI_DRAW_OP_TEXT_BACKGROUND;
    op->data.text.ui_style = ui_style;
    op->data.text.x0 = sx;
    op->data.text.y0 = sy;
    op->data.text.x1 = sx + w;
    op->data.text.y1 = sy + h;
    op->data.text.z = z;
    op->data.text.text_style = text_style;
    M_ScheduleOp(op);
}

void UI_ScheduleDrawTextOutline(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, const int32_t w, const int32_t h,
    const TEXT_STYLE text_style)
{
    M_DRAW_OP *const op = M_AllocDrawOp();
    op->type = UI_DRAW_OP_TEXT_OUTLINE;
    op->data.text.ui_style = ui_style;
    op->data.text.x0 = sx;
    op->data.text.y0 = sy;
    op->data.text.x1 = sx + w;
    op->data.text.y1 = sy + h;
    op->data.text.z = z;
    op->data.text.text_style = text_style;
    M_ScheduleOp(op);
}

void UI_ScheduleDrawScreenSprite(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t scale_h,
    const int32_t scale_v, const int32_t sprite_idx, const int16_t shade)
{
    M_DRAW_OP *const op = M_AllocDrawOp();
    op->type = UI_DRAW_OP_SCREEN_SPRITE;
    op->data.sprite.sx = sx;
    op->data.sprite.sy = sy;
    op->data.sprite.z = z;
    op->data.sprite.scale_h = scale_h;
    op->data.sprite.scale_v = scale_v;
    op->data.sprite.sprite_idx = sprite_idx;
    op->data.sprite.shade = shade;
    M_ScheduleOp(op);
}

void UI_ScheduleDrawScreenFlatQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 color)
{
    M_DRAW_OP *const op = M_AllocDrawOp();
    op->type = UI_DRAW_OP_FLAT_QUAD;
    op->data.flat_quad.sx = sx;
    op->data.flat_quad.sy = sy;
    op->data.flat_quad.z = z;
    op->data.flat_quad.w = w;
    op->data.flat_quad.h = h;
    op->data.flat_quad.color = color;
    M_ScheduleOp(op);
}

void UI_ScheduleDrawScreenGradientQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br)
{
    M_DRAW_OP *const op = M_AllocDrawOp();
    op->type = UI_DRAW_OP_GRADIENT_QUAD;
    op->data.gradient_quad.sx = sx;
    op->data.gradient_quad.sy = sy;
    op->data.gradient_quad.z = z;
    op->data.gradient_quad.w = w;
    op->data.gradient_quad.h = h;
    op->data.gradient_quad.tl = tl;
    op->data.gradient_quad.tr = tr;
    op->data.gradient_quad.bl = bl;
    op->data.gradient_quad.br = br;
    M_ScheduleOp(op);
}

void UI_ScheduleDrawHorizontalLine(
    const UI_STYLE ui_style, const int32_t x0, const int32_t x1,
    const int32_t y, const int32_t z)
{
    M_DRAW_OP *const op = M_AllocDrawOp();
    op->type = UI_DRAW_OP_HORZ_LINE;
    op->data.horz_line.ui_style = ui_style;
    op->data.horz_line.x0 = x0;
    op->data.horz_line.x1 = x1;
    op->data.horz_line.y = y;
    op->data.horz_line.z = z;
    M_ScheduleOp(op);
}

void UI_ScheduleFaderDraw(FADER *const fader)
{
    M_DRAW_OP *const op = M_AllocDrawOp();
    op->type = UI_DRAW_OP_FADER_DRAW;
    op->data.fader.fader = fader;
    M_ScheduleOp(op);
}

void UI_InitDraw(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->ops == nullptr) {
        p->ops = Vector_Create(sizeof(M_DRAW_OP *));
    }
}

void UI_ShutdownDraw(void)
{
    M_PRIV *const p = &m_Priv;
    Memory_ArenaFree(&p->alloc);
    if (p->ops != nullptr) {
        Vector_Free(p->ops);
        p->ops = nullptr;
    }
}

void UI_ClearDraw(void)
{
    M_PRIV *const p = &m_Priv;
    Vector_Clear(p->ops);
    Memory_ArenaReset(&p->alloc);
}

void UI_Draw(void)
{
    M_PRIV *const p = &m_Priv;
    for (int32_t i = 0; i < p->ops->count; i++) {
        const M_DRAW_OP *const op = *(M_DRAW_OP **)Vector_Get(p->ops, i);
        switch (op->type) {
        case UI_DRAW_OP_HORZ_LINE:
            M_DrawHorizontalLine(op);
            break;

        case UI_DRAW_OP_TEXT_BACKGROUND:
            M_DrawTextBackground(
                op->data.text.ui_style, op->data.text.x0, op->data.text.y0,
                op->data.text.x1, op->data.text.y1, op->data.text.z,
                op->data.text.text_style);
            break;

        case UI_DRAW_OP_TEXT_OUTLINE:
            M_DrawTextOutline(
                op->data.text.ui_style, op->data.text.x0, op->data.text.y0,
                op->data.text.x1, op->data.text.y1, op->data.text.z,
                op->data.text.text_style);
            break;

        case UI_DRAW_OP_SCREEN_SPRITE:
            M_DrawScreenSprite(
                op->data.sprite.sx, op->data.sprite.sy, op->data.sprite.z,
                op->data.sprite.scale_h, op->data.sprite.scale_v,
                op->data.sprite.sprite_idx, op->data.sprite.shade);
            break;

        case UI_DRAW_OP_FLAT_QUAD:
            Output_DrawScreenFlatQuad(
                op->data.flat_quad.sx, op->data.flat_quad.sy,
                op->data.flat_quad.z, op->data.flat_quad.w,
                op->data.flat_quad.h, op->data.flat_quad.color);
            break;

        case UI_DRAW_OP_GRADIENT_QUAD:
            Output_DrawScreenGradientQuad(
                op->data.gradient_quad.sx, op->data.gradient_quad.sy,
                op->data.gradient_quad.z, op->data.gradient_quad.w,
                op->data.gradient_quad.h, op->data.gradient_quad.tl,
                op->data.gradient_quad.tr, op->data.gradient_quad.bl,
                op->data.gradient_quad.br);
            break;

        case UI_DRAW_OP_FADER_DRAW:
            Fader_Draw(op->data.fader.fader);
            break;
        }
    }
}
