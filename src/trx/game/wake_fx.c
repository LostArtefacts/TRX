#include <trx/game/wake_fx.h>

#include <trx/game/math.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/utils.h>

#define M_MAX_POINTS 32

static const int32_t m_Offsets[2][2] = { { -128, 0 }, { 128, 0 } };

static WAKE_FX_POINT m_Points[M_MAX_POINTS][2] = {};
static uint8_t m_Shade = 0;
static uint8_t m_StartIndex = 0;

static RGBA_8888 M_GrayFromWakeLife(const int32_t life, const int32_t shift)
{
    int32_t c = (life >> shift) << 3;
    CLAMPG(c, 255);
    return (RGBA_8888) { c, c, c, 255 };
}

static XYZ_32 M_GetWakeOrigin(
    const ITEM *const item, const int32_t x_off, const int32_t z_off)
{
    XYZ_32 pos = item->pos;
    pos = XYZ_32_OffsetYaw(pos, item->rot.y, z_off);
    pos = XYZ_32_OffsetYaw(pos, item->rot.y + DEG_90, x_off);
    return pos;
}

void WakeFX_ClearPoints(void)
{
    for (int32_t i = 0; i < M_MAX_POINTS; i++) {
        m_Points[i][0].life = 0;
        m_Points[i][1].life = 0;
    }
}

void WakeFX_Update(void)
{
    for (int32_t i = 0; i < 2; i++) {
        for (int32_t j = 0; j < M_MAX_POINTS; j++) {
            WAKE_FX_POINT *const pt = &m_Points[j][i];
            if (pt->life) {
                pt->life--;
                pt->pos[0].x += pt->vel[0].x;
                pt->pos[0].z += pt->vel[0].z;
                pt->pos[1].x += pt->vel[1].x;
                pt->pos[1].z += pt->vel[1].z;
            }
        }
    }
}

WAKE_FX_POINT *WakeFX_GetPoint(const int32_t wake_idx, const int32_t side)
{
    return &m_Points[wake_idx][side];
}

uint8_t WakeFX_GetShade(void)
{
    return m_Shade;
}

void WakeFX_SetShade(const uint8_t shade)
{
    m_Shade = shade;
}

uint8_t WakeFX_GetStartIndex(void)
{
    return m_StartIndex;
}

void WakeFX_AdvanceStartIndex(void)
{
    m_StartIndex = (m_StartIndex + 1) & (M_MAX_POINTS - 1);
}

void WakeFX_Draw(const ITEM *const item)
{
    for (int32_t side = 0; side < 2; side++) {
        const XYZ_32 origin =
            M_GetWakeOrigin(item, m_Offsets[side][0], m_Offsets[side][1]);
        XYZ_32 prev[2] = { origin, origin };

        int32_t c12 = 64;
        if (m_Shade < 16) {
            c12 = (c12 * m_Shade) >> 4;
        }

        int32_t current = (m_StartIndex - 1) & (M_MAX_POINTS - 1);
        for (int32_t nw = 0; nw < M_MAX_POINTS; nw++) {
            const WAKE_FX_POINT *const pt = &m_Points[current][side];
            if (pt->life == 0U) {
                break;
            }

            int32_t c34 = pt->life;
            if (m_Shade < 16) {
                c34 = (c34 * m_Shade) >> 4;
            }

            const RGBA_8888 quad_color[4] = {
                M_GrayFromWakeLife(c12, 2),
                M_GrayFromWakeLife(c12, 1),
                M_GrayFromWakeLife(c34, 1),
                M_GrayFromWakeLife(c34, 2),
            };

            XYZ_32 curr[2] = { pt->pos[0], pt->pos[1] };
            const XYZ_32 quad_world[4] = {
                prev[0],
                prev[1],
                curr[0],
                curr[1],
            };

            OutputSource_PolyFX_StageQuadExt(
                -1, quad_world, nullptr, quad_color,
                VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_WIBBLE,
                DRAW_BLEND_ADD);

            prev[0] = curr[1];
            prev[1] = curr[0];
            c12 = c34;
            current = (current - 1) & (M_MAX_POINTS - 1);
        }
    }
}
