#include <trx/game/fx/laser_sight.h>

#include <trx/core/math.h>
#include <trx/game/fx/common.h>
#include <trx/game/interpolation.h>
#include <trx/game/matrix.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/output/vars.h>
#include <trx/game/sparks.h>
#include <trx/game/viewport.h>

// How large the dot is, as half its width on the original's 640-wide screen,
// as the original sizes it (GetFixedScale(3)). It keeps that size on the
// screen at every distance and zoom.
#define M_HALF_SIZE 3.0
#define M_LIGHT_FALLOFF 16
#define M_LIGHT_COLOR ((RGB_888) { .r = 255, .g = 0, .b = 0 })

static bool m_Active = false;
// Where the dot stood at the last two ticks. It moves with the camera, so it
// is drawn between the two at the camera's rate.
static XYZ_32 m_LastPos = {};
static XYZ_32 m_Pos = {};

static XYZ_32 M_GetDrawnPos(void)
{
    const double rate = Interpolation_GetCameraRate();
    return (XYZ_32) {
        .x = m_LastPos.x + (int32_t)((m_Pos.x - m_LastPos.x) * rate),
        .y = m_LastPos.y + (int32_t)((m_Pos.y - m_LastPos.y) * rate),
        .z = m_LastPos.z + (int32_t)((m_Pos.z - m_LastPos.z) * rate),
    };
}

// Draws the dot as the original does: a square of the sprite, facing the
// camera and centred on where the sight points, in front of the scene.
static void M_Draw(void)
{
    if (!m_Active) {
        return;
    }
    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_LASER_SIGHT);
    if (sprite_idx == NO_ITEM) {
        return;
    }
    const XYZ_32 pos = M_GetDrawnPos();
    const MATRIX *const m = &g_ViewMatrix;
    const double w2v = (double)(1 << W2V_SHIFT);
    const double view_dist = ((double)m->_20 * (pos.x - g_ViewPos.x)
                              + (double)m->_21 * (pos.y - g_ViewPos.y)
                              + (double)m->_22 * (pos.z - g_ViewPos.z))
        / w2v;
    if (view_dist <= 0.0) {
        return;
    }
    const double pixels =
        M_HALF_SIZE * Viewport_GetWidth(VIEWPORT_GAME) / 640.0;
    const float h = (float)(pixels * view_dist / g_PhdPersp);
    const XYZ_32 world_pos[4] = { pos, pos, pos, pos };
    const float disp[4][2] = {
        { -h, -h },
        { -h, h },
        { h, h },
        { h, -h },
    };
    const RGBA_8888 red = { .r = 255, .g = 0, .b = 0, .a = 255 };
    const RGBA_8888 colors[4] = { red, red, red, red };
    OutputSource_PolyFX_StageQuadExtDepth(
        sprite_idx, world_pos, disp, colors,
        VERT_NO_LIGHTING | VERT_NO_WIBBLE | VERT_BILLBOARD | VERT_ABS_SPRITE,
        -(float)view_dist, DRAW_BLEND_ADD);
}

static void M_Reset(void)
{
    m_Active = false;
}

void FX_LaserSight_SetDot(const XYZ_32 pos)
{
    m_LastPos = m_Active ? m_Pos : pos;
    m_Active = true;
    m_Pos = pos;
    Output_AddDynamicLightRGB(pos, M_LIGHT_FALLOFF, M_LIGHT_COLOR);
}

void FX_LaserSight_ClearDot(void)
{
    m_Active = false;
}

static const FX_MODULE m_Module = {
    .draw_func = M_Draw,
    .reset_func = M_Reset,
};

REGISTER_FX(m_Module)
