#include <trx/game/fx/wake.h>

#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/math.h>
#include <trx/game/output/sources/poly_fx.h>

#define M_MAX_POINTS 32

static const XZ_32 m_Offsets[2] = { { .x = -128, 0 }, { .x = 128, .z = 0 } };

static FX_WAKE_POINT m_Points[M_MAX_POINTS][2] = {};
static uint8_t m_Shade = 0;
static uint8_t m_StartIndex = 0;

static RGBA_8888 M_GrayFromWakeLife(const int32_t life, const int32_t shift)
{
    int32_t c = (life >> shift) << 3;
    CLAMPG(c, 255);
    return (RGBA_8888) { c, c, c, 255 };
}

static XYZ_32 M_GetWakeOrigin(const ITEM *const item, const XZ_32 offset)
{
    XYZ_32 pos = item->pos;
    pos = XYZ_32_OffsetYaw(pos, item->rot.y, offset.z);
    pos = XYZ_32_OffsetYaw(pos, item->rot.y + DEG_90, offset.x);
    return pos;
}

void FX_Wake_ClearPoints(void)
{
    for (int32_t i = 0; i < M_MAX_POINTS; i++) {
        m_Points[i][0].life = 0;
        m_Points[i][1].life = 0;
    }
}

void FX_Wake_Update(void)
{
    for (int32_t i = 0; i < 2; i++) {
        for (int32_t j = 0; j < M_MAX_POINTS; j++) {
            FX_WAKE_POINT *const pt = &m_Points[j][i];
            if (pt->life > 0) {
                pt->life--;
                pt->pos[0].x += pt->vel[0].x;
                pt->pos[0].z += pt->vel[0].z;
                pt->pos[1].x += pt->vel[1].x;
                pt->pos[1].z += pt->vel[1].z;
            }
        }
    }
}

FX_WAKE_POINT *FX_Wake_GetPoint(const int32_t wake_idx, const int32_t side)
{
    return &m_Points[wake_idx][side];
}

uint8_t FX_Wake_GetShade(void)
{
    return m_Shade;
}

void FX_Wake_SetShade(const uint8_t shade)
{
    m_Shade = shade;
}

uint8_t FX_Wake_GetStartIndex(void)
{
    return m_StartIndex;
}

void FX_Wake_AdvanceStartIndex(void)
{
    m_StartIndex = (m_StartIndex + 1) & (M_MAX_POINTS - 1);
}

void FX_Wake_Draw(const ITEM *const item)
{
    const int64_t max_origin_dist_sq =
        XYZ_32_GetLength2_64((XYZ_32) { WALL_L / 2, 0, 0 });

    for (int32_t side = 0; side < 2; side++) {
        const XYZ_32 origin = M_GetWakeOrigin(item, m_Offsets[side]);
        XYZ_32 prev[2] = { origin, origin };

        int32_t c12 = 64;
        if (m_Shade < 16) {
            c12 = (c12 * m_Shade) >> 4;
        }

        int32_t current = (m_StartIndex - 1) & (M_MAX_POINTS - 1);
        const FX_WAKE_POINT *const first_pt = &m_Points[current][side];
        if (first_pt->life != 0) {
            const XYZ_32 delta0 = {
                .x = origin.x - first_pt->pos[0].x,
                .y = 0,
                .z = origin.z - first_pt->pos[0].z,
            };
            const XYZ_32 delta1 = {
                .x = origin.x - first_pt->pos[1].x,
                .y = 0,
                .z = origin.z - first_pt->pos[1].z,
            };
            const int64_t dist0_sq = XYZ_32_GetLength2_64(delta0);
            const int64_t dist1_sq = XYZ_32_GetLength2_64(delta1);

            if (dist0_sq > max_origin_dist_sq
                && dist1_sq > max_origin_dist_sq) {
                // Prevent a long bridge quad when the hull origin drifts away
                // from the latest wake segment (e.g. moving over dry slopes).
                prev[0] = first_pt->pos[1];
                prev[1] = first_pt->pos[0];
            }
        }

        for (int32_t nw = 0; nw < M_MAX_POINTS; nw++) {
            const FX_WAKE_POINT *const pt = &m_Points[current][side];
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
