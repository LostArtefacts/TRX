#include "game/output/draw.h"

#include "game/inventory_ring.h"
#include "game/level.h"
#include "game/output.h"
#include "game/random.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/const.h>
#include <libtrx/game/creature/const.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output/sources/lightnings.h>
#include <libtrx/game/output/sources/misc.h>
#include <libtrx/game/output/sources/objects.h>
#include <libtrx/game/output/sources/rooms.h>
#include <libtrx/game/output/sources/rooms_debug.h>
#include <libtrx/game/output/sources/shadows.h>
#include <libtrx/game/output/sources/sprites.h>
#include <libtrx/game/output/sources/ui.h>
#include <libtrx/game/output/state.h>
#include <libtrx/game/output/utils.h>
#include <libtrx/game/scaler.h>
#include <libtrx/gfx/context.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>
#include <libtrx/utils.h>

static int16_t m_ShadesTable[32];
static int32_t m_RandomTable[32];

static void M_DrawScreenQuad(
    int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t z, RGBA_8888 tl,
    RGBA_8888 tr, RGBA_8888 bl, RGBA_8888 br);

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

void Output_DrawBlackRectangle(int32_t opacity)
{
    const int32_t sx = 0;
    const int32_t sy = 0;
    const int32_t sw = Viewport_GetWidth(VIEWPORT_UI);
    const int32_t sh = Viewport_GetHeight(VIEWPORT_UI);
    const RGBA_8888 background = { 0, 0, 0, opacity };
    Output_DrawScreenFlatQuad(sx, sy, 0, sw, sh, background);
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

void Output_DrawTextOutline(
    const UI_STYLE ui_style, const int32_t x, const int32_t y, const int32_t z,
    const int32_t width, const int32_t height, const TEXT_STYLE text_style)
{
    const int32_t mesh_idx = Object_Get(O_TEXT_BOX)->mesh_idx;

    const int32_t offset = 4;
    const int32_t x0 = x + offset;
    const int32_t y0 = y + offset;
    const int32_t x1 = x0 + width - offset * 2;
    const int32_t y1 = y0 + height - offset * 2;
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

    int32_t w = (width - offset * 2) * PHD_ONE / 8;
    int32_t h = (height - offset * 2) * PHD_ONE / 8;

    Output_DrawScreenSprite(x0, y0, z, w, scale_v, mesh_idx + 4, SHADE_NEUTRAL);
    Output_DrawScreenSprite(x1, y0, z, scale_h, h, mesh_idx + 5, SHADE_NEUTRAL);
    Output_DrawScreenSprite(x0, y1, z, w, scale_v, mesh_idx + 6, SHADE_NEUTRAL);
    Output_DrawScreenSprite(x0, y0, z, scale_h, h, mesh_idx + 7, SHADE_NEUTRAL);
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
}

void Output_DrawObjectMesh(const OBJECT_MESH *const mesh, const CLIP clip)
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

void Output_DrawObjectMesh_I(const OBJECT_MESH *const mesh, const CLIP clip)
{
    Matrix_Push();
    Matrix_Interpolate();
    Output_DrawObjectMesh(mesh, clip);
    Matrix_Pop();
}

void Output_DrawRoom(const ROOM *const room, const bool is_outside)
{
    OutputSource_Rooms_StageRoom(room);
    if (g_Config.debug.enable_debug_triggers
        || g_Config.debug.enable_debug_portals) {
        OutputSource_RoomsDebug_StageRoom(room);
    }
}

void Output_DrawSkybox(const OBJECT_MESH *const mesh)
{
    OutputSource_Objects_StageSkyboxMesh(
        mesh,
        0x1000 + 0x400 * Output_GetSunsetTimer() / Output_GetSunsetDuration());
}

void Output_DrawSprite(
    const uint32_t flags, int32_t x, int32_t y, int32_t z,
    const int16_t sprite_idx, int16_t shade, const int16_t scale)
{
    Matrix_Push();
    Matrix_TranslateAbs(x, y, z);
    OutputSource_Sprites_Stage(sprite_idx, shade, (RGB_F) { 1.0f, 1.0f, 1.0f });
    Matrix_Pop();
}

void Output_DrawScreenSprite(
    const int32_t sx, const int32_t sy, const int32_t sz, const int32_t scale_h,
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
        .z = Output_GetNearZ_UI() + sz,
        .shade = shade,
        .color = { 255, 255, 255, 255 },
    });
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
