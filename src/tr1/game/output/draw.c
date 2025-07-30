#include "game/output.h"
#include "game/output/sources/lightnings.h"
#include "game/output/sources/misc.h"
#include "game/output/sources/objects.h"
#include "game/output/sources/rooms.h"
#include "game/output/sources/rooms_debug.h"
#include "game/output/sources/sprites.h"
#include "game/output/sources/ui.h"
#include "game/output/utils.h"

#include <libtrx/config.h>
#include <libtrx/memory.h>

#define M_TEXT_OUTLINE_THICKNESS 2

typedef enum {
    MC_PURPLE_C,
    MC_PURPLE_E,
    MC_BROWN_C,
    MC_BROWN_E,
    MC_GREY_C,
    MC_GREY_E,
    MC_GREY_TL,
    MC_GREY_TR,
    MC_GREY_BL,
    MC_GREY_BR,
    MC_BLACK,
    MC_GOLD_LIGHT,
    MC_GOLD_DARK,
    MC_NUMBER_OF,
} MENU_COLOR;

static RGBA_8888 m_MenuColorMap[MC_NUMBER_OF] = {
    // clang-format off
    [MC_PURPLE_C]   = { 70,  30,  107, 230 },
    [MC_PURPLE_E]   = { 70,  30,  107, 0 },
    [MC_BROWN_C]    = { 91,  46,  9,   255 },
    [MC_BROWN_E]    = { 91,  46,  9,   0 },
    [MC_GREY_C]     = { 197, 197, 197, 255 },
    [MC_GREY_E]     = { 45,  45,  45,  255 },
    [MC_GREY_TL]    = { 96,  96,  96,  255 },
    [MC_GREY_TR]    = { 32,  32,  32,  255 },
    [MC_GREY_BL]    = { 63,  63,  63,  255 },
    [MC_GREY_BR]    = { 0,   0,   0,   255 },
    [MC_BLACK]      = { 0,   0,   0,   255 },
    [MC_GOLD_LIGHT] = { 232, 192, 112, 255 },
    [MC_GOLD_DARK]  = { 140, 112, 56,  255 },
    // clang-format on
};

static RGBA_8888 M_GetMenuColor(MENU_COLOR color);
static void M_DrawScreenQuad(
    int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t z, RGBA_8888 tl,
    RGBA_8888 tr, RGBA_8888 bl, RGBA_8888 br);

static RGBA_8888 M_GetMenuColor(const MENU_COLOR color)
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

void Output_DrawSkybox(const OBJECT_MESH *const mesh)
{
    OutputSource_Objects_StageSkyboxMesh(mesh);
}

void Output_DrawObjectMesh(const OBJECT_MESH *const mesh, const int32_t clip)
{
    OutputSource_Objects_StageObjectMesh(mesh);
    if (g_Config.debug.enable_debug_spheres) {
        Matrix_Push();
        Matrix_TranslateRel16(mesh->center);
        Matrix_Scale(mesh->radius << W2V_SHIFT);
        OutputSource_Misc_StageSphere();
        Matrix_Pop();
    }
}

void Output_DrawObjectMesh_I(const OBJECT_MESH *const mesh, const int32_t clip)
{
    Matrix_Push();
    Matrix_Interpolate();
    Output_DrawObjectMesh(mesh, clip);
    Matrix_Pop();
}

void Output_DrawRoomMesh(ROOM *const room)
{
    Output_LightRoom(room);
    OutputSource_Rooms_StageRoom(room);
    if (g_Config.debug.enable_debug_triggers
        || g_Config.debug.enable_debug_portals) {
        OutputSource_RoomsDebug_StageRoom(room);
    }
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
        item->interp.result.pos.x, item->floor, item->interp.result.pos.z);
    Matrix_RotY(item->rot.y);
    Matrix_TranslateRel(x_mid, 0, z_mid);
    Matrix_ScaleX((1 << W2V_SHIFT) * x_size / UNIT_SHADOW);
    Matrix_ScaleZ((1 << W2V_SHIFT) * z_size / UNIT_SHADOW);
    OutputSource_Misc_StageShadow();
    Matrix_Pop();
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

void Output_DrawTextOutline(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, int32_t w, int32_t h, const TEXT_STYLE text_style)
{
    if (ui_style == UI_STYLE_PC) {
        Output_DrawScreenFrame(
            sx, sy, w, h, M_GetMenuColor(MC_GOLD_DARK),
            M_GetMenuColor(MC_GOLD_LIGHT), M_TEXT_OUTLINE_THICKNESS);
        return;
    }

    if (text_style == TS_HEADING) {
        Output_DrawScreenGradientBox(
            sx, sy, w, h, M_GetMenuColor(MC_BLACK), M_GetMenuColor(MC_BLACK),
            M_GetMenuColor(MC_BLACK), M_GetMenuColor(MC_BLACK),
            M_TEXT_OUTLINE_THICKNESS);
    } else if (text_style == TS_BACKGROUND) {
        Output_DrawScreenGradientBox(
            sx, sy, w, h, M_GetMenuColor(MC_GREY_TL),
            M_GetMenuColor(MC_GREY_TR), M_GetMenuColor(MC_GREY_BL),
            M_GetMenuColor(MC_GREY_BR), M_TEXT_OUTLINE_THICKNESS);
    } else if (text_style == TS_REQUESTED) {
        // Make sure height and width divisible by 2.
        w = 2 * ((w + 1) / 2);
        h = 2 * ((h + 1) / 2);
        Output_DrawScreenCentreGradientBox(
            sx, sy, w, h, M_GetMenuColor(MC_GREY_E), M_GetMenuColor(MC_GREY_C),
            M_TEXT_OUTLINE_THICKNESS);
    }
}

void Output_DrawTextBackground(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, int32_t w, int32_t h, const TEXT_STYLE text_style)
{
    const RGBA_8888 cb = { 0, 0, 0,
                           text_style == TS_BACKGROUND_HEAVY ? 224 : 128 };

    // Make sure height and width divisible by 2.
    w = 2 * ((w + 1) / 2);
    h = 2 * ((h + 1) / 2);
    M_DrawScreenQuad(sx, sy, sx + w, sy + h, z, cb, cb, cb, cb);

    if (ui_style == UI_STYLE_PC) {
        return;
    }

    RGBA_8888 cc, ce;
    switch (text_style) {
    case TS_HEADING:
        cc = M_GetMenuColor(MC_BROWN_C);
        ce = M_GetMenuColor(MC_BROWN_E);
        break;
    case TS_REQUESTED:
        cc = M_GetMenuColor(MC_PURPLE_C);
        ce = M_GetMenuColor(MC_PURPLE_E);
        break;
    default:
        return;
    }

    const int32_t x0 = sx;
    const int32_t y0 = sy;
    const int32_t x1 = sx + w;
    const int32_t y1 = sy + h;
    const int32_t xm = sx + w / 2;
    const int32_t ym = sy + h / 2;
    M_DrawScreenQuad(x0, y0, xm, ym, z, ce, ce, ce, cc);
    M_DrawScreenQuad(x1, y0, xm, ym, z, ce, ce, ce, cc);
    M_DrawScreenQuad(x0, y1, xm, ym, z, ce, ce, ce, cc);
    M_DrawScreenQuad(x1, y1, xm, ym, z, ce, ce, ce, cc);
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
    const RGBA_8888 br, const int32_t thickness)
{
    const float scale = Viewport_GetHeight(VIEWPORT_UI) / 480.0;
    const float x0 = sx - scale;
    const float y0 = sy - scale;
    const float x1 = sx + w + scale;
    const float y1 = sy + h + scale;
    const float e = thickness * scale / 2.0f;
    M_DrawScreenQuad(x0 - e, y0 - e, x1 + e, y0 + e, 0, tl, tr, tl, tr);
    M_DrawScreenQuad(x0 - e, y1 - e, x1 + e, y1 + e, 0, bl, br, bl, br);
    M_DrawScreenQuad(x0 - e, y0 - e, x0 + e, y1 + e, 0, tl, tl, bl, bl);
    M_DrawScreenQuad(x1 - e, y0 - e, x1 + e, y1 + e, 0, tr, tr, br, br);
}

void Output_DrawScreenCentreGradientBox(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 edge, const RGBA_8888 center, const int32_t thickness)
{
    const float scale = Viewport_GetHeight(VIEWPORT_UI) / 480.0f;
    const float x0 = sx - scale;
    const float y0 = sy - scale;
    const float x1 = sx + w + scale;
    const float y1 = sy + h + scale;
    const float e = thickness * scale / 2.0f;
    const float xm = (x0 + x1) / 2.0f;
    const float ym = (y0 + y1) / 2.0f;
    const RGBA_8888 cc = center;
    const RGBA_8888 ce = edge;

    // clang-format off
    M_DrawScreenQuad(x0 - e, y0 - e, xm,     y0 + e, 0, ce, cc, ce, cc);
    M_DrawScreenQuad(xm,     y0 - e, x1 + e, y0 + e, 0, cc, ce, cc, ce);
    M_DrawScreenQuad(x0 - e, y1 - e, xm,     y1 + e, 0, ce, cc, ce, cc);
    M_DrawScreenQuad(xm,     y1 - e, x1 + e, y1 + e, 0, cc, ce, cc, ce);
    M_DrawScreenQuad(x0 - e, y0,     x0 + e, ym,     0, ce, ce, cc, cc);
    M_DrawScreenQuad(x0 - e, ym,     x0 + e, y1,     0, cc, cc, ce, ce);
    M_DrawScreenQuad(x1 - e, y0,     x1 + e, ym,     0, ce, ce, cc, cc);
    M_DrawScreenQuad(x1 - e, ym,     x1 + e, y1,     0, cc, cc, ce, ce);
    // clang-format on
}

void Output_DrawScreenFrame(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 col_dark, const RGBA_8888 col_light,
    const int32_t thickness)
{
    const float scale = Viewport_GetHeight(VIEWPORT_UI) / 480.0f;
    const float e = thickness * scale / 2.0f;
    const float x0 = sx - scale;
    const float y0 = sy - scale;
    const float x1 = sx + w + scale;
    const float y1 = sy + h + scale;
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

void Output_DrawBlackRectangle(const int32_t opacity)
{
    const int32_t sx = 0;
    const int32_t sy = 0;
    const int32_t sw = Viewport_GetWidth(VIEWPORT_UI);
    const int32_t sh = Viewport_GetHeight(VIEWPORT_UI);
    const RGBA_8888 background = { 0, 0, 0, opacity };
    Output_DrawScreenFlatQuad(sx, sy, 0, sw, sh, background);
}
