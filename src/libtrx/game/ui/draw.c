#include "game/ui/draw.h"

#include "game/output/common.h"
#include "memory.h"
#include "vector.h"

#include <string.h>

// Draw operation types for deferred UI rendering.
typedef enum {
    UI_DRAW_OP_TEXT_BACKGROUND,
    UI_DRAW_OP_TEXT_OUTLINE,
    UI_DRAW_OP_SCREEN_SPRITE,
    UI_DRAW_OP_FLAT_QUAD,
    UI_DRAW_OP_GRADIENT_QUAD,
    UI_DRAW_OP_FADER_DRAW,
} M_DRAW_OP_TYPE;

// Deferred draw operation node.
typedef struct {
    M_DRAW_OP_TYPE type;
    union {
        struct {
            UI_STYLE ui_style;
            int32_t sx, sy, z, w, h;
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
    op->data.text.sx = sx;
    op->data.text.sy = sy;
    op->data.text.z = z;
    op->data.text.w = w;
    op->data.text.h = h;
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
    op->data.text.sx = sx;
    op->data.text.sy = sy;
    op->data.text.w = w;
    op->data.text.h = h;
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
    if (p->ops == nullptr) {
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
        case UI_DRAW_OP_TEXT_BACKGROUND:
            Output_DrawTextBackground(
                op->data.text.ui_style, op->data.text.sx, op->data.text.sy,
                op->data.text.z, op->data.text.w, op->data.text.h,
                op->data.text.text_style);
            break;
        case UI_DRAW_OP_TEXT_OUTLINE:
            Output_DrawTextOutline(
                op->data.text.ui_style, op->data.text.sx, op->data.text.sy,
                op->data.text.z, op->data.text.w, op->data.text.h,
                op->data.text.text_style);
            break;
        case UI_DRAW_OP_SCREEN_SPRITE:
            Output_DrawScreenSprite(
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
