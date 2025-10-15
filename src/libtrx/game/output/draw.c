#include "game/output/draw.h"

#include "config.h"
#include "game/creature/const.h"
#include "game/objects.h"
#include "game/output.h"
#include "game/output/sources/lightnings.h"
#include "game/output/sources/misc.h"
#include "game/output/sources/objects.h"
#include "game/output/sources/rooms.h"
#include "game/output/sources/rooms_debug.h"
#include "game/output/sources/shadows.h"
#include "game/output/sources/sprites.h"
#include "game/output/sources/ui.h"
#include "game/output/state.h"
#include "game/scaler.h"
#include "game/shell.h"

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

static M_GRADIENT_FILL m_GradientFills[] = {
    // clang-format off
    [TS_BACKGROUND]       = { .edge = C_BACKGROUND_E, .center = C_BACKGROUND_C },
    [TS_BACKGROUND_HEAVY] = { .edge = C_BACKGROUND_HEAVY_E, .center = C_BACKGROUND_HEAVY_C },
    [TS_HEADING]          = { .edge = C_HEADING_E, .center = C_HEADING_C },
    [TS_REQUESTED]        = { .edge = C_REQUESTED_E, .center = C_REQUESTED_C },
    // clang-format on
};

static RGBA_8888 m_MenuColorMap[C_NUMBER_OF] = {
// clang-format off
#if TR_VERSION == 1
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
#elif TR_VERSION == 2
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
#endif
    [C_BACKGROUND_OUTLINE_TL]  = { 0x60, 0x60, 0x60, 0xFF },
    [C_BACKGROUND_OUTLINE_TR]  = { 0x20, 0x20, 0x20, 0xFF },
    [C_BACKGROUND_OUTLINE_BL]  = { 0x40, 0x40, 0x40, 0xFF },
    [C_BACKGROUND_OUTLINE_BR]  = { 0x00, 0x00, 0x00, 0xFF },
    [C_HEADING_OUTLINE]        = { 0x00, 0x00, 0x00, 0xFF },
    [C_GENERIC_OUTLINE_LIGHT]  = { 0xE8, 0xC0, 0x70, 0xFF },
    [C_GENERIC_OUTLINE_DARK]   = { 0x8C, 0x70, 0x38, 0xFF },
    // clang-format on
};

static RGBA_8888 M_GetMenuColor(const M_COLOR color)
{
    return m_MenuColorMap[color];
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

void Output_DrawBlackRectangle(const int32_t opacity)
{
    const int32_t sx = 0;
    const int32_t sy = 0;
    const int32_t sw = Viewport_GetWidth(VIEWPORT_UI);
    const int32_t sh = Viewport_GetHeight(VIEWPORT_UI);
    const RGBA_8888 background = { 0, 0, 0, opacity };
    Output_DrawScreenFlatQuad(sx, sy, 0, sw, sh, background);
}

void Output_DrawRoom(const ROOM *const room, const bool is_outside)
{
    OutputSource_Rooms_StageRoom(room);
    if (g_Config.debug.enable_debug_triggers
        || g_Config.debug.enable_debug_portals) {
        OutputSource_RoomsDebug_StageRoom(room);
    }
}

void Output_DrawSprite(
    const int32_t x, const int32_t y, const int32_t z, const int16_t sprite_idx,
    const int16_t shade, const RGB_F tint)
{
    Matrix_Push();
    Matrix_TranslateAbs(x, y, z);
    OutputSource_Sprites_Stage(sprite_idx, shade, tint);
    Matrix_Pop();
}

void Output_DrawObjectMesh(const OBJECT_MESH *const mesh, const CLIP clip)
{
    OutputSource_Objects_StageObjectMesh(mesh);
    if (g_Config.debug.enable_debug_spheres) {
        Output_DrawSphere(mesh->center, mesh->radius);
    }
}

void Output_DrawObjectMesh_I(const OBJECT_MESH *const mesh, const CLIP clip)
{
    Matrix_Push();
    Matrix_Interpolate();
    Output_DrawObjectMesh(mesh, clip);
    Matrix_Pop();
}

void Output_DrawSkybox(const OBJECT_MESH *const mesh)
{
    OutputSource_Objects_StageSkyboxMesh(
        mesh,
        0x1000 + 0x400 * Output_GetSunsetTimer() / Output_GetSunsetDuration());
}

void Output_DrawShadow(
    const int16_t size, const BOUNDS_16 *const bounds, const ITEM *const item)
{
    if (!item->enable_shadow) {
        return;
    }

    const int32_t x_0 = bounds->min.x;
    const int32_t x_1 = bounds->max.x;
    const int32_t z_0 = bounds->min.z;
    const int32_t z_1 = bounds->max.z;
    const int32_t x_mid = (x_0 + x_1) / 2;
    const int32_t z_mid = (z_0 + z_1) / 2;
    const int32_t x_size = (x_1 - x_0) * size / 1024;
    const int32_t z_size = (z_1 - z_0) * size / 1024;

    Matrix_Push();
    *g_MatrixPtr = g_W2VMatrix;
    Matrix_TranslateAbs(
        item->interp.result.pos.x, item->interp.result.floor,
        item->interp.result.pos.z);
    Matrix_RotY(item->rot.y);
    Matrix_TranslateRel(x_mid, 0, z_mid);
    Matrix_ScaleX((1 << W2V_SHIFT) * x_size / UNIT_SHADOW);
    Matrix_ScaleZ((1 << W2V_SHIFT) * z_size / UNIT_SHADOW);
    OutputSource_Shadows_StageShadow();
    Matrix_Pop();
}

void Output_DrawLightningSegment(const LIGHTNING_SEGMENT segment)
{
    OutputSource_Lightnings_StageSegment(&segment);
}

void Output_DrawScreenSprite(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t scale_h,
    const int32_t scale_v, const int32_t sprite_idx, const int16_t shade)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);
    const int32_t x0 = sx + (scale_h * sprite->x0 / PHD_ONE);
    const int32_t x1 = sx + (scale_h * sprite->x1 / PHD_ONE);
    const int32_t y0 = sy + (scale_v * sprite->y0 / PHD_ONE);
    const int32_t y1 = sy + (scale_v * sprite->y1 / PHD_ONE);
    OutputSource_UI_StageSprite((OUTPUT_UI_SPRITE) {
        .sprite_idx = sprite_idx,
        .x0 = x0,
        .y0 = y0,
        .x1 = x1,
        .y1 = y1,
        .z = Output_GetNearZ_UI() + z,
        .shade = shade,
        .color = { 255, 255, 255, 255 },
    });
}

void Output_DrawTextBackground(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, int32_t w, int32_t h, const TEXT_STYLE text_style)
{
    // Make sure height and width divisible by 2.
    w = 2 * ((w + 1) / 2);
    h = 2 * ((h + 1) / 2);

    if (ui_style == UI_STYLE_PC) {
        const RGBA_8888 cb = { 0, 0, 0,
                               text_style == TS_BACKGROUND_HEAVY ? 224 : 128 };
        M_DrawScreenQuad(sx, sy, sx + w, sy + h, z, cb, cb, cb, cb);
        return;
    }

    const int32_t x0 = sx;
    const int32_t y0 = sy;
    const int32_t x1 = sx + w;
    const int32_t y1 = sy + h;
    const int32_t xm = sx + w / 2;
    const int32_t ym = sy + h / 2;

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

void Output_DrawTextOutline(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, int32_t w, int32_t h, const TEXT_STYLE text_style)
{
#if TR_VERSION == 2
    if (ui_style == UI_STYLE_PC) {
        const int32_t mesh_idx = Object_Get(O_TEXT_BOX)->mesh_idx;

        const int32_t offset = 4;
        const int32_t x0 = sx + offset;
        const int32_t y0 = sy + offset;
        const int32_t x1 = x0 + w - offset * 2;
        const int32_t y1 = y0 + h - offset * 2;
        const int32_t scale_h = PHD_ONE;
        const int32_t scale_v = PHD_ONE;

        Output_DrawScreenSprite(
            x0, y0, z, scale_h, scale_v, mesh_idx + 0, SHADE_NEUTRAL);
        Output_DrawScreenSprite(
            x1, y0, z, scale_h, scale_v, mesh_idx + 1, SHADE_NEUTRAL);
        Output_DrawScreenSprite(
            x1, y1, z, scale_h, scale_v, mesh_idx + 2, SHADE_NEUTRAL);
        Output_DrawScreenSprite(
            x0, y1, z, scale_h, scale_v, mesh_idx + 3, SHADE_NEUTRAL);

        w = (w - offset * 2) * PHD_ONE / 8;
        h = (h - offset * 2) * PHD_ONE / 8;

        Output_DrawScreenSprite(
            x0, y0, z, w, scale_v, mesh_idx + 4, SHADE_NEUTRAL);
        Output_DrawScreenSprite(
            x1, y0, z, scale_h, h, mesh_idx + 5, SHADE_NEUTRAL);
        Output_DrawScreenSprite(
            x0, y1, z, w, scale_v, mesh_idx + 6, SHADE_NEUTRAL);
        Output_DrawScreenSprite(
            x0, y0, z, scale_h, h, mesh_idx + 7, SHADE_NEUTRAL);
        return;
    }
#else
    if (ui_style == UI_STYLE_PC) {
        Output_DrawScreenFrame(
            sx, sy, w, h, M_GetMenuColor(C_GENERIC_OUTLINE_DARK),
            M_GetMenuColor(C_GENERIC_OUTLINE_LIGHT), M_OUTLINE_THICKNESS);
        return;
    }
#endif

    if (text_style == TS_HEADING) {
        Output_DrawScreenGradientBox(
            sx, sy, w, h, M_GetMenuColor(C_HEADING_OUTLINE),
            M_GetMenuColor(C_HEADING_OUTLINE),
            M_GetMenuColor(C_HEADING_OUTLINE),
            M_GetMenuColor(C_HEADING_OUTLINE), M_OUTLINE_THICKNESS);
    } else if (
        text_style == TS_BACKGROUND || text_style == TS_BACKGROUND_HEAVY) {
        Output_DrawScreenGradientBox(
            sx, sy, w, h, M_GetMenuColor(C_BACKGROUND_OUTLINE_TL),
            M_GetMenuColor(C_BACKGROUND_OUTLINE_TR),
            M_GetMenuColor(C_BACKGROUND_OUTLINE_BL),
            M_GetMenuColor(C_BACKGROUND_OUTLINE_BR), M_OUTLINE_THICKNESS);
    } else if (text_style == TS_REQUESTED) {
        // Make sure height and width divisible by 2.
        w = 2 * ((w + 1) / 2);
        h = 2 * ((h + 1) / 2);
        Output_DrawScreenCentreGradientBox(
            sx, sy, w, h, M_GetMenuColor(C_REQUESTED_OUTLINE_E),
            M_GetMenuColor(C_REQUESTED_OUTLINE_CH),
            M_GetMenuColor(C_REQUESTED_OUTLINE_CV), M_OUTLINE_THICKNESS);
    }
}

void Output_DrawScreenFlatQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 color)
{
    M_DrawScreenQuad(sx, sy, sx + w, sy + h, z, color, color, color, color);
}

void Output_DrawScreenGradientQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br)
{
    M_DrawScreenQuad(sx, sy, sx + w, sy + h, z, tl, tr, bl, br);
}

void Output_DrawScreenGradientBox(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br, const float thickness)
{
    const float e = Scaler_Calc(thickness, SCALER_TARGET_TEXT);
    const float x0 = sx;
    const float y0 = sy;
    const float x1 = sx + w;
    const float y1 = sy + h;
    M_DrawScreenQuad(x0 - e, y0 - e, x1 + e, y0 + e, 0, tl, tr, tl, tr);
    M_DrawScreenQuad(x0 - e, y1 - e, x1 + e, y1 + e, 0, bl, br, bl, br);
    M_DrawScreenQuad(x0 - e, y0 - e, x0 + e, y1 + e, 0, tl, tl, bl, bl);
    M_DrawScreenQuad(x1 - e, y0 - e, x1 + e, y1 + e, 0, tr, tr, br, br);
}

void Output_DrawScreenCentreGradientBox(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 edge, const RGBA_8888 center_h, const RGBA_8888 center_v,
    const float thickness)
{
    const float e = Scaler_Calc(thickness, SCALER_TARGET_TEXT);
    const float x0 = sx;
    const float y0 = sy;
    const float x1 = sx + w;
    const float y1 = sy + h;
    const float xm = (x0 + x1) / 2.0f;
    const float ym = (y0 + y1) / 2.0f;
    const RGBA_8888 ch = center_h;
    const RGBA_8888 cv = center_v;
    const RGBA_8888 ce = edge;
    const RGBA_8888 cc = (RGBA_8888) { 255, 0, 0, 255 };

    // clang-format off
    M_DrawScreenQuad(x0 - e, y0 - e, xm,     y0 + e, 0, ce, ch, ce, ch);
    M_DrawScreenQuad(xm,     y0 - e, x1 + e, y0 + e, 0, ch, ce, ch, ce);
    M_DrawScreenQuad(x0 - e, y1 - e, xm,     y1 + e, 0, ce, ch, ce, ch);
    M_DrawScreenQuad(xm,     y1 - e, x1 + e, y1 + e, 0, ch, ce, ch, ce);
    M_DrawScreenQuad(x0 - e, y0,     x0 + e, ym,     0, ce, ce, cv, cv);
    M_DrawScreenQuad(x0 - e, ym,     x0 + e, y1,     0, cv, cv, ce, ce);
    M_DrawScreenQuad(x1 - e, y0,     x1 + e, ym,     0, ce, ce, cv, cv);
    M_DrawScreenQuad(x1 - e, ym,     x1 + e, y1,     0, cv, cv, ce, ce);
    // clang-format on
}

void Output_DrawScreenFrame(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 col_dark, const RGBA_8888 col_light, const float thickness)
{
    const float e = Scaler_Calc(thickness, SCALER_TARGET_TEXT);
    const float x0 = sx;
    const float y0 = sy;
    const float x1 = sx + w;
    const float y1 = sy + h;
    const RGBA_8888 cd = col_dark;
    const RGBA_8888 cl = col_light;

    // clang-format off
    M_DrawScreenQuad(x0,     y0,     x1 - e, y0 + e, 0, cd, cd, cd, cd);
    M_DrawScreenQuad(x0 - e, y0 - e, x1,     y0,     0, cl, cl, cl, cl);
    M_DrawScreenQuad(x1,     y0 - e, x1 + e, y1 + e, 0, cd, cd, cd, cd);
    M_DrawScreenQuad(x1 - e, y0,     x1,     y1,     0, cl, cl, cl, cl);
    M_DrawScreenQuad(x0,     y0,     x0 + e, y1 - e, 0, cd, cd, cd, cd);
    M_DrawScreenQuad(x0 - e, y0 - e, x0,     y1,     0, cl, cl, cl, cl);
    M_DrawScreenQuad(x0 - e, y1,     x1 + e, y1 + e, 0, cd, cd, cd, cd);
    M_DrawScreenQuad(x0 - e, y1 - e, x1,     y1,     0, cl, cl, cl, cl);
    // clang-format on
}

void Output_DrawSphere(const XYZ_16 center, const int32_t radius)
{
    Matrix_Push();
    Matrix_TranslateRel16(center);
    Matrix_Scale(radius << W2V_SHIFT);
    OutputSource_Misc_StageSphere();
    Matrix_Pop();
}

void Output_DrawCuboid(const BOUNDS_16 *const bounds)
{
    const int32_t x0 = bounds->min.x;
    const int32_t x1 = bounds->max.x;
    const int32_t y0 = bounds->min.y;
    const int32_t y1 = bounds->max.y;
    const int32_t z0 = bounds->min.z;
    const int32_t z1 = bounds->max.z;
    const int32_t x_mid = (x0 + x1) / 2;
    const int32_t y_mid = (y0 + y1) / 2;
    const int32_t z_mid = (z0 + z1) / 2;
    const int32_t x_size = (x1 - x0) / 2;
    const int32_t y_size = (y1 - y0) / 2;
    const int32_t z_size = (z1 - z0) / 2;
    Matrix_Push();
    Matrix_TranslateRel32((XYZ_32) { x_mid, y_mid, z_mid });
    Matrix_ScaleX(x_size << W2V_SHIFT);
    Matrix_ScaleY(y_size << W2V_SHIFT);
    Matrix_ScaleZ(z_size << W2V_SHIFT);
    OutputSource_Misc_StageCuboid();
    Matrix_Pop();
}
