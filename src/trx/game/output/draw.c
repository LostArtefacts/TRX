#include <trx/game/output/draw.h>

#include <trx/config.h>
#include <trx/core/math/geom.h>
#include <trx/core/utils.h>
#include <trx/game/creature/const.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/output/bind.h>
#include <trx/game/output/const.h>
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

// The largest half-extent a matrix scale holds, the shift into it included.
// A shape reaching further than this draws at this size rather than wrapping
// round into a small one or an inside-out one.
#define M_MAX_SHAPE_EXTENT (INT32_MAX >> W2V_SHIFT)

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
        .z = OUTPUT_UI_NEAR_Z + z,
    });
}

// The middle and the half-extent of one axis of a shape, computed wide because
// a span reaching both ends of the coordinate range does not fit 32 bits.
static int32_t M_ShapeMid(const int32_t lo, const int32_t hi)
{
    return (int32_t)(((int64_t)lo + hi) / 2);
}

static int32_t M_ShapeExtent(const int32_t lo, const int32_t hi)
{
    const int64_t extent = ((int64_t)hi - lo) / 2;
    return (int32_t)MIN(extent, M_MAX_SHAPE_EXTENT);
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
        Output_DrawSphere(XYZ_32_From16(mesh->center), mesh->radius);
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
        .z = OUTPUT_UI_NEAR_Z + z,
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

void Output_DrawSphere(const XYZ_32 center, const int32_t radius)
{
    const bool wireframe_state = g_Config.rendering.enable_wireframe;
    const RGBA_8888 color_black = { 0, 0, 0, 128 };
    const RGBA_8888 color_white = { 255, 255, 255, 128 };
    const RGBA_8888 color = wireframe_state ? color_black : color_white;
    Output_DrawSphereEx(center, radius, color);
}

void Output_DrawSphereEx(
    const XYZ_32 center, const int32_t radius, const RGBA_8888 color)
{
    int32_t extent = radius;
    CLAMPG(extent, M_MAX_SHAPE_EXTENT);
    Matrix_Push();
    Matrix_TranslateRel32(center);
    Matrix_Scale(extent << W2V_SHIFT);
    OutputSource_Misc_StageSphere(color);
    Matrix_Pop();
}

void Output_DrawCuboid(const BOUNDS_32 *const bounds)
{
    Output_DrawCuboidEx(bounds, (RGBA_8888) { 255, 0, 0, 255 });
}

void Output_DrawCuboidEx(const BOUNDS_32 *const bounds, const RGBA_8888 color)
{
    const XYZ_32 mid = {
        .x = M_ShapeMid(bounds->min.x, bounds->max.x),
        .y = M_ShapeMid(bounds->min.y, bounds->max.y),
        .z = M_ShapeMid(bounds->min.z, bounds->max.z),
    };
    const XYZ_32 size = {
        .x = M_ShapeExtent(bounds->min.x, bounds->max.x),
        .y = M_ShapeExtent(bounds->min.y, bounds->max.y),
        .z = M_ShapeExtent(bounds->min.z, bounds->max.z),
    };
    Matrix_Push();
    Matrix_TranslateRel32(mid);
    Matrix_ScaleX(size.x << W2V_SHIFT);
    Matrix_ScaleY(size.y << W2V_SHIFT);
    Matrix_ScaleZ(size.z << W2V_SHIFT);
    OutputSource_Misc_StageCuboid(color);
    Matrix_Pop();
}
