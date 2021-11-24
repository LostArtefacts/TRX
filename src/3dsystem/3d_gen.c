#include "3dsystem/3d_gen.h"

#include "3dsystem/phd_math.h"
#include "3dsystem/scalespr.h"
#include "config.h"
#include "game/screen.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"
#include "specific/s_hwr.h"

#include <math.h>

#define TRIGMULT2(A, B) (((A) * (B)) >> W2V_SHIFT)
#define TRIGMULT3(A, B, C) (TRIGMULT2((TRIGMULT2(A, B)), C))
#define EXTRACT_ROT_Y(rots) (((rots >> 10) & 0x3FF) << 6)
#define EXTRACT_ROT_X(rots) (((rots >> 20) & 0x3FF) << 6)
#define EXTRACT_ROT_Z(rots) ((rots & 0x3FF) << 6)

static PHD_VECTOR m_LsVectorView = { 0 };
static PHD_MATRIX m_MatrixStack[MAX_MATRICES] = { 0 };
static int32_t m_DrawDistFade = 0;
static int32_t m_DrawDistMax = 0;

void phd_GenerateW2V(PHD_3DPOS *viewpos)
{
    g_PhdMatrixPtr = &m_MatrixStack[0];
    int32_t sx = phd_sin(viewpos->x_rot);
    int32_t cx = phd_cos(viewpos->x_rot);
    int32_t sy = phd_sin(viewpos->y_rot);
    int32_t cy = phd_cos(viewpos->y_rot);
    int32_t sz = phd_sin(viewpos->z_rot);
    int32_t cz = phd_cos(viewpos->z_rot);

    m_MatrixStack[0]._00 = TRIGMULT3(sx, sy, sz) + TRIGMULT2(cy, cz);
    m_MatrixStack[0]._01 = TRIGMULT2(cx, sz);
    m_MatrixStack[0]._02 = TRIGMULT3(sx, cy, sz) - TRIGMULT2(sy, cz);
    m_MatrixStack[0]._10 = TRIGMULT3(sx, sy, cz) - TRIGMULT2(cy, sz);
    m_MatrixStack[0]._11 = TRIGMULT2(cx, cz);
    m_MatrixStack[0]._12 = TRIGMULT3(sx, cy, cz) + TRIGMULT2(sy, sz);
    m_MatrixStack[0]._20 = TRIGMULT2(cx, sy);
    m_MatrixStack[0]._21 = -sx;
    m_MatrixStack[0]._22 = TRIGMULT2(cx, cy);
    m_MatrixStack[0]._03 = viewpos->x;
    m_MatrixStack[0]._13 = viewpos->y;
    m_MatrixStack[0]._23 = viewpos->z;
    g_W2VMatrix = m_MatrixStack[0];
}

void phd_LookAt(
    int32_t xsrc, int32_t ysrc, int32_t zsrc, int32_t xtar, int32_t ytar,
    int32_t ztar, int16_t roll)
{
    PHD_ANGLE angles[2];
    phd_GetVectorAngles(xtar - xsrc, ytar - ysrc, ztar - zsrc, angles);

    PHD_3DPOS viewer;
    viewer.x = xsrc;
    viewer.y = ysrc;
    viewer.z = zsrc;
    viewer.x_rot = angles[1];
    viewer.y_rot = angles[0];
    viewer.z_rot = roll;
    phd_GenerateW2V(&viewer);
}

void phd_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest)
{
    dest[0] = phd_atan(z, x);

    while ((int16_t)x != x || (int16_t)y != y || (int16_t)z != z) {
        x >>= 2;
        y >>= 2;
        z >>= 2;
    }

    PHD_ANGLE pitch = phd_atan(phd_sqrt(SQUARE(x) + SQUARE(z)), y);
    if ((y > 0 && pitch > 0) || (y < 0 && pitch < 0)) {
        pitch = -pitch;
    }

    dest[1] = pitch;
}

void phd_RotX(PHD_ANGLE rx)
{
    if (!rx) {
        return;
    }

    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    int32_t sx = phd_sin(rx);
    int32_t cx = phd_cos(rx);

    int32_t r0, r1;
    r0 = mptr->_01 * cx + mptr->_02 * sx;
    r1 = mptr->_02 * cx - mptr->_01 * sx;
    mptr->_01 = r0 >> W2V_SHIFT;
    mptr->_02 = r1 >> W2V_SHIFT;

    r0 = mptr->_11 * cx + mptr->_12 * sx;
    r1 = mptr->_12 * cx - mptr->_11 * sx;
    mptr->_11 = r0 >> W2V_SHIFT;
    mptr->_12 = r1 >> W2V_SHIFT;

    r0 = mptr->_21 * cx + mptr->_22 * sx;
    r1 = mptr->_22 * cx - mptr->_21 * sx;
    mptr->_21 = r0 >> W2V_SHIFT;
    mptr->_22 = r1 >> W2V_SHIFT;
}

void phd_RotY(PHD_ANGLE ry)
{
    if (!ry) {
        return;
    }

    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    int32_t sy = phd_sin(ry);
    int32_t cy = phd_cos(ry);

    int32_t r0, r1;
    r0 = mptr->_00 * cy - mptr->_02 * sy;
    r1 = mptr->_02 * cy + mptr->_00 * sy;
    mptr->_00 = r0 >> W2V_SHIFT;
    mptr->_02 = r1 >> W2V_SHIFT;

    r0 = mptr->_10 * cy - mptr->_12 * sy;
    r1 = mptr->_12 * cy + mptr->_10 * sy;
    mptr->_10 = r0 >> W2V_SHIFT;
    mptr->_12 = r1 >> W2V_SHIFT;

    r0 = mptr->_20 * cy - mptr->_22 * sy;
    r1 = mptr->_22 * cy + mptr->_20 * sy;
    mptr->_20 = r0 >> W2V_SHIFT;
    mptr->_22 = r1 >> W2V_SHIFT;
}

void phd_RotZ(PHD_ANGLE rz)
{
    if (!rz) {
        return;
    }

    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    int32_t sz = phd_sin(rz);
    int32_t cz = phd_cos(rz);

    int32_t r0, r1;
    r0 = mptr->_00 * cz + mptr->_01 * sz;
    r1 = mptr->_01 * cz - mptr->_00 * sz;
    mptr->_00 = r0 >> W2V_SHIFT;
    mptr->_01 = r1 >> W2V_SHIFT;

    r0 = mptr->_10 * cz + mptr->_11 * sz;
    r1 = mptr->_11 * cz - mptr->_10 * sz;
    mptr->_10 = r0 >> W2V_SHIFT;
    mptr->_11 = r1 >> W2V_SHIFT;

    r0 = mptr->_20 * cz + mptr->_21 * sz;
    r1 = mptr->_21 * cz - mptr->_20 * sz;
    mptr->_20 = r0 >> W2V_SHIFT;
    mptr->_21 = r1 >> W2V_SHIFT;
}

void phd_RotYXZ(PHD_ANGLE ry, PHD_ANGLE rx, PHD_ANGLE rz)
{
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    int32_t r0, r1;

    if (ry) {
        int32_t sy = phd_sin(ry);
        int32_t cy = phd_cos(ry);

        r0 = mptr->_00 * cy - mptr->_02 * sy;
        r1 = mptr->_02 * cy + mptr->_00 * sy;
        mptr->_00 = r0 >> W2V_SHIFT;
        mptr->_02 = r1 >> W2V_SHIFT;

        r0 = mptr->_10 * cy - mptr->_12 * sy;
        r1 = mptr->_12 * cy + mptr->_10 * sy;
        mptr->_10 = r0 >> W2V_SHIFT;
        mptr->_12 = r1 >> W2V_SHIFT;

        r0 = mptr->_20 * cy - mptr->_22 * sy;
        r1 = mptr->_22 * cy + mptr->_20 * sy;
        mptr->_20 = r0 >> W2V_SHIFT;
        mptr->_22 = r1 >> W2V_SHIFT;
    }

    if (rx) {
        int32_t sx = phd_sin(rx);
        int32_t cx = phd_cos(rx);

        r0 = mptr->_01 * cx + mptr->_02 * sx;
        r1 = mptr->_02 * cx - mptr->_01 * sx;
        mptr->_01 = r0 >> W2V_SHIFT;
        mptr->_02 = r1 >> W2V_SHIFT;

        r0 = mptr->_11 * cx + mptr->_12 * sx;
        r1 = mptr->_12 * cx - mptr->_11 * sx;
        mptr->_11 = r0 >> W2V_SHIFT;
        mptr->_12 = r1 >> W2V_SHIFT;

        r0 = mptr->_21 * cx + mptr->_22 * sx;
        r1 = mptr->_22 * cx - mptr->_21 * sx;
        mptr->_21 = r0 >> W2V_SHIFT;
        mptr->_22 = r1 >> W2V_SHIFT;
    }

    if (rz) {
        int32_t sz = phd_sin(rz);
        int32_t cz = phd_cos(rz);

        r0 = mptr->_00 * cz + mptr->_01 * sz;
        r1 = mptr->_01 * cz - mptr->_00 * sz;
        mptr->_00 = r0 >> W2V_SHIFT;
        mptr->_01 = r1 >> W2V_SHIFT;

        r0 = mptr->_10 * cz + mptr->_11 * sz;
        r1 = mptr->_11 * cz - mptr->_10 * sz;
        mptr->_10 = r0 >> W2V_SHIFT;
        mptr->_11 = r1 >> W2V_SHIFT;

        r0 = mptr->_20 * cz + mptr->_21 * sz;
        r1 = mptr->_21 * cz - mptr->_20 * sz;
        mptr->_20 = r0 >> W2V_SHIFT;
        mptr->_21 = r1 >> W2V_SHIFT;
    }
}

void phd_RotYXZpack(int32_t rots)
{
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    int32_t r0, r1;

    PHD_ANGLE ry = EXTRACT_ROT_Y(rots);
    if (ry) {
        int32_t sy = phd_sin(ry);
        int32_t cy = phd_cos(ry);

        r0 = mptr->_00 * cy - mptr->_02 * sy;
        r1 = mptr->_02 * cy + mptr->_00 * sy;
        mptr->_00 = r0 >> W2V_SHIFT;
        mptr->_02 = r1 >> W2V_SHIFT;

        r0 = mptr->_10 * cy - mptr->_12 * sy;
        r1 = mptr->_12 * cy + mptr->_10 * sy;
        mptr->_10 = r0 >> W2V_SHIFT;
        mptr->_12 = r1 >> W2V_SHIFT;

        r0 = mptr->_20 * cy - mptr->_22 * sy;
        r1 = mptr->_22 * cy + mptr->_20 * sy;
        mptr->_20 = r0 >> W2V_SHIFT;
        mptr->_22 = r1 >> W2V_SHIFT;
    }

    PHD_ANGLE rx = EXTRACT_ROT_X(rots);
    if (rx) {
        int32_t sx = phd_sin(rx);
        int32_t cx = phd_cos(rx);

        r0 = mptr->_01 * cx + mptr->_02 * sx;
        r1 = mptr->_02 * cx - mptr->_01 * sx;
        mptr->_01 = r0 >> W2V_SHIFT;
        mptr->_02 = r1 >> W2V_SHIFT;

        r0 = mptr->_11 * cx + mptr->_12 * sx;
        r1 = mptr->_12 * cx - mptr->_11 * sx;
        mptr->_11 = r0 >> W2V_SHIFT;
        mptr->_12 = r1 >> W2V_SHIFT;

        r0 = mptr->_21 * cx + mptr->_22 * sx;
        r1 = mptr->_22 * cx - mptr->_21 * sx;
        mptr->_21 = r0 >> W2V_SHIFT;
        mptr->_22 = r1 >> W2V_SHIFT;
    }

    PHD_ANGLE rz = EXTRACT_ROT_Z(rots);
    if (rz) {
        int32_t sz = phd_sin(rz);
        int32_t cz = phd_cos(rz);

        r0 = mptr->_00 * cz + mptr->_01 * sz;
        r1 = mptr->_01 * cz - mptr->_00 * sz;
        mptr->_00 = r0 >> W2V_SHIFT;
        mptr->_01 = r1 >> W2V_SHIFT;

        r0 = mptr->_10 * cz + mptr->_11 * sz;
        r1 = mptr->_11 * cz - mptr->_10 * sz;
        mptr->_10 = r0 >> W2V_SHIFT;
        mptr->_11 = r1 >> W2V_SHIFT;

        r0 = mptr->_20 * cz + mptr->_21 * sz;
        r1 = mptr->_21 * cz - mptr->_20 * sz;
        mptr->_20 = r0 >> W2V_SHIFT;
        mptr->_21 = r1 >> W2V_SHIFT;
    }
}

int32_t phd_TranslateRel(int32_t x, int32_t y, int32_t z)
{
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    mptr->_03 += mptr->_00 * x + mptr->_01 * y + mptr->_02 * z;
    mptr->_13 += mptr->_10 * x + mptr->_11 * y + mptr->_12 * z;
    mptr->_23 += mptr->_20 * x + mptr->_21 * y + mptr->_22 * z;
    return ABS(mptr->_03) <= phd_GetFarZ() && ABS(mptr->_13) <= phd_GetFarZ()
        && ABS(mptr->_23) <= phd_GetFarZ();
}

void phd_TranslateAbs(int32_t x, int32_t y, int32_t z)
{
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    x -= g_W2VMatrix._03;
    y -= g_W2VMatrix._13;
    z -= g_W2VMatrix._23;
    mptr->_03 = mptr->_00 * x + mptr->_01 * y + mptr->_02 * z;
    mptr->_13 = mptr->_10 * x + mptr->_11 * y + mptr->_12 * z;
    mptr->_23 = mptr->_20 * x + mptr->_21 * y + mptr->_22 * z;
}

int32_t phd_VisibleZClip(PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3)
{
    double v1x = vn1->xv;
    double v1y = vn1->yv;
    double v1z = vn1->zv;
    double v2x = vn2->xv;
    double v2y = vn2->yv;
    double v2z = vn2->zv;
    double v3x = vn3->xv;
    double v3y = vn3->yv;
    double v3z = vn3->zv;
    double a = v3y * v1x - v1y * v3x;
    double b = v3x * v1z - v1x * v3z;
    double c = v3z * v1y - v1z * v3y;
    return a * v2z + b * v2y + c * v2x < 0.0;
}

void phd_RotateLight(int16_t pitch, int16_t yaw)
{
    int32_t cp = phd_cos(pitch);
    int32_t sp = phd_sin(pitch);
    int32_t cy = phd_cos(yaw);
    int32_t sy = phd_sin(yaw);
    int32_t ls_x = TRIGMULT2(cp, sy);
    int32_t ls_y = -sp;
    int32_t ls_z = TRIGMULT2(cp, cy);
    m_LsVectorView.x = (g_W2VMatrix._00 * ls_x + g_W2VMatrix._01 * ls_y
                        + g_W2VMatrix._02 * ls_z)
        >> W2V_SHIFT;
    m_LsVectorView.y = (g_W2VMatrix._10 * ls_x + g_W2VMatrix._11 * ls_y
                        + g_W2VMatrix._12 * ls_z)
        >> W2V_SHIFT;
    m_LsVectorView.z = (g_W2VMatrix._20 * ls_x + g_W2VMatrix._21 * ls_y
                        + g_W2VMatrix._22 * ls_z)
        >> W2V_SHIFT;
}

void phd_ResetMatrixStack()
{
    g_PhdMatrixPtr = &m_MatrixStack[0];
}

void phd_AlterFOV(PHD_ANGLE fov)
{
    // In places that use GAME_FOV, it can be safely changed to user's choice.
    // But for cinematics, the FOV value chosen by devs needs to stay
    // unchanged, otherwise the game renders the low camera in the Lost Valley
    // cutscene wrong.
    if (g_Config.fov_vertical) {
        double aspect_ratio =
            Screen_GetResWidth() / (double)Screen_GetResHeight();
        double fov_rad_h = fov * M_PI / 32760;
        double fov_rad_v = 2 * atan(aspect_ratio * tan(fov_rad_h / 2));
        fov = round((fov_rad_v / M_PI) * 32760);
    }

    int16_t c = phd_cos(fov / 2);
    int16_t s = phd_sin(fov / 2);
    g_PhdPersp = ((Screen_GetResWidth() / 2) * c) / s;
}

void phd_PushMatrix()
{
    g_PhdMatrixPtr++;
    g_PhdMatrixPtr[0] = g_PhdMatrixPtr[-1];
}

void phd_PushUnitMatrix()
{
    PHD_MATRIX *mptr = ++g_PhdMatrixPtr;
    mptr->_00 = W2V_SCALE;
    mptr->_01 = 0;
    mptr->_02 = 0;
    mptr->_10 = 0;
    mptr->_11 = W2V_SCALE;
    mptr->_12 = 0;
    mptr->_20 = 0;
    mptr->_21 = 0;
    mptr->_22 = W2V_SCALE;
}

void phd_PopMatrix()
{
    g_PhdMatrixPtr--;
}

const int16_t *calc_object_vertices(const int16_t *obj_ptr)
{
    int16_t total_clip = -1;

    obj_ptr++;
    int vertex_count = *obj_ptr++;
    for (int i = 0; i < vertex_count; i++) {
        int32_t xv = g_PhdMatrixPtr->_00 * obj_ptr[0]
            + g_PhdMatrixPtr->_01 * obj_ptr[1]
            + g_PhdMatrixPtr->_02 * obj_ptr[2] + g_PhdMatrixPtr->_03;
        int32_t yv = g_PhdMatrixPtr->_10 * obj_ptr[0]
            + g_PhdMatrixPtr->_11 * obj_ptr[1]
            + g_PhdMatrixPtr->_12 * obj_ptr[2] + g_PhdMatrixPtr->_13;
        int32_t zv = g_PhdMatrixPtr->_20 * obj_ptr[0]
            + g_PhdMatrixPtr->_21 * obj_ptr[1]
            + g_PhdMatrixPtr->_22 * obj_ptr[2] + g_PhdMatrixPtr->_23;
        g_PhdVBuf[i].xv = xv;
        g_PhdVBuf[i].yv = yv;
        g_PhdVBuf[i].zv = zv;

        int32_t clip_flags;
        if (zv < phd_GetNearZ()) {
            clip_flags = -32768;
        } else {
            clip_flags = 0;

            int32_t xs = ViewPort_GetCenterX() + xv / (zv / g_PhdPersp);
            int32_t ys = ViewPort_GetCenterY() + yv / (zv / g_PhdPersp);

            if (xs < g_PhdLeft) {
                if (xs < -32760) {
                    xs = -32760;
                }
                clip_flags |= 1;
            } else if (xs > g_PhdRight) {
                if (xs > 32760) {
                    xs = 32760;
                }
                clip_flags |= 2;
            }

            if (ys < g_PhdTop) {
                if (ys < -32760) {
                    ys = -32760;
                }
                clip_flags |= 4;
            } else if (ys > g_PhdBottom) {
                if (ys > 32760) {
                    ys = 32760;
                }
                clip_flags |= 8;
            }

            g_PhdVBuf[i].xs = xs;
            g_PhdVBuf[i].ys = ys;
        }

        g_PhdVBuf[i].clip = clip_flags;
        total_clip &= clip_flags;
        obj_ptr += 3;
    }

    return total_clip == 0 ? obj_ptr : NULL;
}

const int16_t *calc_vertice_light(const int16_t *obj_ptr)
{
    int32_t vertex_count = *obj_ptr++;
    if (vertex_count > 0) {
        if (g_LsDivider) {
            int32_t xv = (g_PhdMatrixPtr->_00 * m_LsVectorView.x
                          + g_PhdMatrixPtr->_10 * m_LsVectorView.y
                          + g_PhdMatrixPtr->_20 * m_LsVectorView.z)
                / g_LsDivider;
            int32_t yv = (g_PhdMatrixPtr->_01 * m_LsVectorView.x
                          + g_PhdMatrixPtr->_11 * m_LsVectorView.y
                          + g_PhdMatrixPtr->_21 * m_LsVectorView.z)
                / g_LsDivider;
            int32_t zv = (g_PhdMatrixPtr->_02 * m_LsVectorView.x
                          + g_PhdMatrixPtr->_12 * m_LsVectorView.y
                          + g_PhdMatrixPtr->_22 * m_LsVectorView.z)
                / g_LsDivider;
            for (int i = 0; i < vertex_count; i++) {
                int16_t shade = g_LsAdder
                    + ((obj_ptr[0] * xv + obj_ptr[1] * yv + obj_ptr[2] * zv)
                       >> 16);
                CLAMP(shade, 0, 0x1FFF);
                g_PhdVBuf[i].g = shade;
                obj_ptr += 3;
            }
            return obj_ptr;
        } else {
            int16_t shade = g_LsAdder;
            CLAMP(shade, 0, 0x1FFF);
            for (int i = 0; i < vertex_count; i++) {
                g_PhdVBuf[i].g = shade;
            }
            obj_ptr += 3 * vertex_count;
        }
    } else {
        for (int i = 0; i < -vertex_count; i++) {
            int16_t shade = g_LsAdder + *obj_ptr++;
            CLAMP(shade, 0, 0x1FFF);
            g_PhdVBuf[i].g = shade;
        }
    }
    return obj_ptr;
}

const int16_t *calc_roomvert(const int16_t *obj_ptr)
{
    int32_t vertex_count = *obj_ptr++;

    for (int i = 0; i < vertex_count; i++) {
        int32_t xv = g_PhdMatrixPtr->_00 * obj_ptr[0]
            + g_PhdMatrixPtr->_01 * obj_ptr[1]
            + g_PhdMatrixPtr->_02 * obj_ptr[2] + g_PhdMatrixPtr->_03;
        int32_t yv = g_PhdMatrixPtr->_10 * obj_ptr[0]
            + g_PhdMatrixPtr->_11 * obj_ptr[1]
            + g_PhdMatrixPtr->_12 * obj_ptr[2] + g_PhdMatrixPtr->_13;
        int32_t zv = g_PhdMatrixPtr->_20 * obj_ptr[0]
            + g_PhdMatrixPtr->_21 * obj_ptr[1]
            + g_PhdMatrixPtr->_22 * obj_ptr[2] + g_PhdMatrixPtr->_23;
        g_PhdVBuf[i].xv = xv;
        g_PhdVBuf[i].yv = yv;
        g_PhdVBuf[i].zv = zv;
        g_PhdVBuf[i].g = obj_ptr[3];

        if (zv < phd_GetNearZ()) {
            g_PhdVBuf[i].clip = 0x8000;
        } else {
            int16_t clip_flags = 0;
            int32_t depth = zv >> W2V_SHIFT;
            if (depth > phd_GetDrawDistMax()) {
                g_PhdVBuf[i].g = 0x1FFF;
                clip_flags |= 16;
            } else if (depth) {
                g_PhdVBuf[i].g += phd_CalculateFogShade(depth);
                if (!g_IsWaterEffect) {
                    CLAMPG(g_PhdVBuf[i].g, 0x1FFF);
                }
            }

            int32_t xs = ViewPort_GetCenterX() + xv / (zv / g_PhdPersp);
            int32_t ys = ViewPort_GetCenterY() + yv / (zv / g_PhdPersp);
            if (g_IsWibbleEffect) {
                xs += g_WibbleTable[(ys + g_WibbleOffset) & 0x1F];
                ys += g_WibbleTable[(xs + g_WibbleOffset) & 0x1F];
            }

            if (xs < g_PhdLeft) {
                if (xs < -32760) {
                    xs = -32760;
                }
                clip_flags |= 1;
            } else if (xs > g_PhdRight) {
                if (xs > 32760) {
                    xs = 32760;
                }
                clip_flags |= 2;
            }

            if (ys < g_PhdTop) {
                if (ys < -32760) {
                    ys = -32760;
                }
                clip_flags |= 4;
            } else if (ys > g_PhdBottom) {
                if (ys > 32760) {
                    ys = 32760;
                }
                clip_flags |= 8;
            }

            if (g_IsWaterEffect) {
                g_PhdVBuf[i].g += g_ShadeTable[(
                    ((uint8_t)g_WibbleOffset
                     + (uint8_t)g_RandTable[(vertex_count - i) % WIBBLE_SIZE])
                    % WIBBLE_SIZE)];
                CLAMP(g_PhdVBuf[i].g, 0, 0x1FFF);
            }

            g_PhdVBuf[i].xs = xs;
            g_PhdVBuf[i].ys = ys;
            g_PhdVBuf[i].clip = clip_flags;
        }
        obj_ptr += 4;
    }

    return obj_ptr;
}

void phd_InitPolyList()
{
    HWR_InitPolyList();
}

void phd_PutPolygons(const int16_t *obj_ptr, int clip)
{
    obj_ptr += 4;
    obj_ptr = calc_object_vertices(obj_ptr);
    if (obj_ptr) {
        obj_ptr = calc_vertice_light(obj_ptr);
        obj_ptr = HWR_InsertObjectGT4(obj_ptr + 1, *obj_ptr);
        obj_ptr = HWR_InsertObjectGT3(obj_ptr + 1, *obj_ptr);
        obj_ptr = HWR_InsertObjectG4(obj_ptr + 1, *obj_ptr);
        obj_ptr = HWR_InsertObjectG3(obj_ptr + 1, *obj_ptr);
    }
}

void S_InsertRoom(const int16_t *obj_ptr)
{
    obj_ptr = calc_roomvert(obj_ptr);
    obj_ptr = HWR_InsertObjectGT4(obj_ptr + 1, *obj_ptr);
    obj_ptr = HWR_InsertObjectGT3(obj_ptr + 1, *obj_ptr);
    obj_ptr = S_DrawRoomSprites(obj_ptr + 1, *obj_ptr);
}

int32_t phd_GetDrawDistMin()
{
    return 127;
}

int32_t phd_GetDrawDistFade()
{
    return m_DrawDistFade;
}

int32_t phd_GetDrawDistMax()
{
    return m_DrawDistMax;
}

void phd_SetDrawDistFade(int32_t dist)
{
    m_DrawDistFade = dist;
}

void phd_SetDrawDistMax(int32_t dist)
{
    m_DrawDistMax = dist;
}

int32_t phd_GetNearZ()
{
    return phd_GetDrawDistMin() << W2V_SHIFT;
}

int32_t phd_GetFarZ()
{
    return phd_GetDrawDistMax() << W2V_SHIFT;
}

int32_t phd_CalculateFogShade(int32_t depth)
{
    int32_t fog_begin = phd_GetDrawDistFade();
    int32_t fog_end = phd_GetDrawDistMax();

    if (depth < fog_begin) {
        return 0;
    }
    if (depth >= fog_end) {
        return 0x1FFF;
    }

    return (depth - fog_begin) * 0x1FFF / (fog_end - fog_begin);
}
