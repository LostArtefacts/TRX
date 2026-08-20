#include <trx/game/output/draw.h>

#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/game/creature/const.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/output/bind.h>
#include <trx/game/output/sources/lightnings.h>
#include <trx/game/output/sources/misc.h>
#include <trx/game/output/sources/objects.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/output/sources/rooms.h>
#include <trx/game/output/sources/rooms_debug.h>
#include <trx/game/output/sources/shadows.h>
#include <trx/game/output/sources/sprites.h>
#include <trx/game/output/sources/ui.h>
#include <trx/game/output/state.h>
#include <trx/game/rooms.h>
#include <trx/game/shell.h>
#include <trx/version.h>

static void M_DrawScreenQuad(
    const float x0, const float y0, const float x1, const float y1,
    const float z, const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
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

void Output_DrawRoom(const ROOM *const room, const bool is_outside)
{
    OutputSource_Rooms_StageRoom(room);
    OutputSource_RoomsDebug_StageRoom(room);
}

void Output_DrawSprite(
    const int32_t x, const int32_t y, const int32_t z, const int16_t sprite_idx,
    const int16_t shade, const RGBA_F tint, const DRAW_TYPE draw_type,
    const float scale)
{
    Matrix_Push();
    Matrix_TranslateAbs(x, y, z);
    if (scale != 1.0f) {
        Matrix_Scale(scale * (1 << W2V_SHIFT));
    }
    OutputSource_Sprites_Stage(sprite_idx, shade, tint, draw_type);
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

void Output_DrawLightningSegment(const LIGHTNING_SEGMENT segment)
{
    OutputSource_Lightnings_StageSegment(&segment);
}

void Output_DrawScreenSprite(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t scale_h,
    const int32_t scale_v, const int32_t sprite_idx, const RGBA_F colors[4])
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
        .color = {
            colors[0],
            colors[1],
            colors[2],
            colors[3],
        },
    });
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

void Output_DrawScreenFrame(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 col_dark, const RGBA_8888 col_light, const float thickness)
{
    const float e = thickness;
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

void Output_DrawPhotoModeFrame(const int32_t thickness)
{
    const VIEWPORT_RECT rect = Viewport_GetRect(VIEWPORT_UI);
    const RGBA_8888 color = { 255, 0, 0, 96 };
    OutputSource_UI_StagePhotoModeFrame(rect, color, thickness);
}

void Output_DrawSphere(const XYZ_16 center, const int32_t radius)
{
    const bool wireframe_state = g_Config.rendering.enable_wireframe;
    const RGBA_8888 color_black = { 0, 0, 0, 128 };
    const RGBA_8888 color_white = { 255, 255, 255, 128 };
    const RGBA_8888 color = wireframe_state ? color_black : color_white;
    Output_DrawSphereEx(center, radius, color);
}

void Output_DrawSphereEx(
    const XYZ_16 center, const int32_t radius, const RGBA_8888 color)
{
    Matrix_Push();
    Matrix_TranslateRel16(center);
    Matrix_Scale(radius << W2V_SHIFT);
    OutputSource_Misc_StageSphere(color);
    Matrix_Pop();
}

void Output_DrawCuboid(const BOUNDS_16 *const bounds)
{
    Output_DrawCuboidEx(bounds, (RGBA_8888) { 255, 0, 0, 255 });
}

void Output_DrawCuboidEx(const BOUNDS_16 *const bounds, const RGBA_8888 color)
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
    OutputSource_Misc_StageCuboid(color);
    Matrix_Pop();
}
