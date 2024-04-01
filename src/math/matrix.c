#include "math/matrix.h"

#include "global/vars.h"
#include "math/math.h"
#include "math/math_misc.h"

#include <stddef.h>

#define EXTRACT_ROT_Y(rots) (((rots >> 10) & 0x3FF) << 6)
#define EXTRACT_ROT_X(rots) (((rots >> 20) & 0x3FF) << 6)
#define EXTRACT_ROT_Z(rots) ((rots & 0x3FF) << 6)

static MATRIX m_MatrixStack[MAX_MATRICES] = { 0 };
static int32_t m_IMRate = 0;
static int32_t m_IMFrac = 0;
static MATRIX *m_IMMatrixPtr = NULL;
static MATRIX m_IMMatrixStack[MAX_NESTED_MATRICES] = { 0 };

void Matrix_ResetStack(void)
{
    g_MatrixPtr = &m_MatrixStack[0];
}

void Matrix_GenerateW2V(const XYZ_32 *pos, const XYZ_16 *rot)
{
    g_MatrixPtr = &m_MatrixStack[0];
    int32_t sx = Math_Sin(rot->x);
    int32_t cx = Math_Cos(rot->x);
    int32_t sy = Math_Sin(rot->y);
    int32_t cy = Math_Cos(rot->y);
    int32_t sz = Math_Sin(rot->z);
    int32_t cz = Math_Cos(rot->z);

    m_MatrixStack[0]._00 = TRIGMULT3(sx, sy, sz) + TRIGMULT2(cy, cz);
    m_MatrixStack[0]._01 = TRIGMULT2(cx, sz);
    m_MatrixStack[0]._02 = TRIGMULT3(sx, cy, sz) - TRIGMULT2(sy, cz);
    m_MatrixStack[0]._10 = TRIGMULT3(sx, sy, cz) - TRIGMULT2(cy, sz);
    m_MatrixStack[0]._11 = TRIGMULT2(cx, cz);
    m_MatrixStack[0]._12 = TRIGMULT3(sx, cy, cz) + TRIGMULT2(sy, sz);
    m_MatrixStack[0]._20 = TRIGMULT2(cx, sy);
    m_MatrixStack[0]._21 = -sx;
    m_MatrixStack[0]._22 = TRIGMULT2(cx, cy);
    m_MatrixStack[0]._03 = pos->x;
    m_MatrixStack[0]._13 = pos->y;
    m_MatrixStack[0]._23 = pos->z;
    g_W2VMatrix = m_MatrixStack[0];
}

bool Matrix_Push(void)
{
    if (g_MatrixPtr + 1 - m_MatrixStack >= MAX_MATRICES) {
        return false;
    }
    g_MatrixPtr++;
    g_MatrixPtr[0] = g_MatrixPtr[-1];
    return true;
}

bool Matrix_PushUnit(void)
{
    if (g_MatrixPtr + 1 - m_MatrixStack >= MAX_MATRICES) {
        return false;
    }
    MATRIX *mptr = ++g_MatrixPtr;
    mptr->_00 = W2V_SCALE;
    mptr->_01 = 0;
    mptr->_02 = 0;
    mptr->_10 = 0;
    mptr->_11 = W2V_SCALE;
    mptr->_12 = 0;
    mptr->_20 = 0;
    mptr->_21 = 0;
    mptr->_22 = W2V_SCALE;
    mptr->_03 = 0;
    mptr->_13 = 0;
    mptr->_23 = 0;
    return true;
}

void Matrix_Pop(void)
{
    g_MatrixPtr--;
}

void Matrix_RotX(PHD_ANGLE rx)
{
    if (!rx) {
        return;
    }

    MATRIX *mptr = g_MatrixPtr;
    int32_t sx = Math_Sin(rx);
    int32_t cx = Math_Cos(rx);

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

void Matrix_RotY(PHD_ANGLE ry)
{
    if (!ry) {
        return;
    }

    MATRIX *mptr = g_MatrixPtr;
    int32_t sy = Math_Sin(ry);
    int32_t cy = Math_Cos(ry);

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

void Matrix_RotZ(PHD_ANGLE rz)
{
    if (!rz) {
        return;
    }

    MATRIX *mptr = g_MatrixPtr;
    int32_t sz = Math_Sin(rz);
    int32_t cz = Math_Cos(rz);

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

void Matrix_RotYXZ(PHD_ANGLE ry, PHD_ANGLE rx, PHD_ANGLE rz)
{
    MATRIX *mptr = g_MatrixPtr;
    int32_t r0, r1;

    if (ry) {
        int32_t sy = Math_Sin(ry);
        int32_t cy = Math_Cos(ry);

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
        int32_t sx = Math_Sin(rx);
        int32_t cx = Math_Cos(rx);

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
        int32_t sz = Math_Sin(rz);
        int32_t cz = Math_Cos(rz);

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

void Matrix_RotYXZpack(int32_t rots)
{
    MATRIX *mptr = g_MatrixPtr;
    int32_t r0, r1;

    PHD_ANGLE ry = EXTRACT_ROT_Y(rots);
    if (ry) {
        int32_t sy = Math_Sin(ry);
        int32_t cy = Math_Cos(ry);

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
        int32_t sx = Math_Sin(rx);
        int32_t cx = Math_Cos(rx);

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
        int32_t sz = Math_Sin(rz);
        int32_t cz = Math_Cos(rz);

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

void Matrix_TranslateRel(int32_t x, int32_t y, int32_t z)
{
    MATRIX *mptr = g_MatrixPtr;
    mptr->_03 += mptr->_00 * x + mptr->_01 * y + mptr->_02 * z;
    mptr->_13 += mptr->_10 * x + mptr->_11 * y + mptr->_12 * z;
    mptr->_23 += mptr->_20 * x + mptr->_21 * y + mptr->_22 * z;
}

void Matrix_TranslateAbs(int32_t x, int32_t y, int32_t z)
{
    MATRIX *mptr = g_MatrixPtr;
    x -= g_W2VMatrix._03;
    y -= g_W2VMatrix._13;
    z -= g_W2VMatrix._23;
    mptr->_03 = mptr->_00 * x + mptr->_01 * y + mptr->_02 * z;
    mptr->_13 = mptr->_10 * x + mptr->_11 * y + mptr->_12 * z;
    mptr->_23 = mptr->_20 * x + mptr->_21 * y + mptr->_22 * z;
}

void Matrix_TranslateSet(int32_t x, int32_t y, int32_t z)
{
    MATRIX *mptr = g_MatrixPtr;
    mptr->_03 = x << W2V_SHIFT;
    mptr->_13 = y << W2V_SHIFT;
    mptr->_23 = z << W2V_SHIFT;
}

void Matrix_InitInterpolate(int32_t frac, int32_t rate)
{
    m_IMFrac = frac;
    m_IMRate = rate;
    m_IMMatrixPtr = &m_IMMatrixStack[0];
    m_IMMatrixPtr->_00 = g_MatrixPtr->_00;
    m_IMMatrixPtr->_01 = g_MatrixPtr->_01;
    m_IMMatrixPtr->_02 = g_MatrixPtr->_02;
    m_IMMatrixPtr->_03 = g_MatrixPtr->_03;
    m_IMMatrixPtr->_10 = g_MatrixPtr->_10;
    m_IMMatrixPtr->_11 = g_MatrixPtr->_11;
    m_IMMatrixPtr->_12 = g_MatrixPtr->_12;
    m_IMMatrixPtr->_13 = g_MatrixPtr->_13;
    m_IMMatrixPtr->_20 = g_MatrixPtr->_20;
    m_IMMatrixPtr->_21 = g_MatrixPtr->_21;
    m_IMMatrixPtr->_22 = g_MatrixPtr->_22;
    m_IMMatrixPtr->_23 = g_MatrixPtr->_23;
}

void Matrix_Interpolate(void)
{
    MATRIX *mptr = g_MatrixPtr;
    MATRIX *iptr = m_IMMatrixPtr;

    mptr->_00 += ((iptr->_00 - mptr->_00) * m_IMFrac) / m_IMRate;
    mptr->_01 += ((iptr->_01 - mptr->_01) * m_IMFrac) / m_IMRate;
    mptr->_02 += ((iptr->_02 - mptr->_02) * m_IMFrac) / m_IMRate;
    mptr->_03 += ((iptr->_03 - mptr->_03) * m_IMFrac) / m_IMRate;
    mptr->_10 += ((iptr->_10 - mptr->_10) * m_IMFrac) / m_IMRate;
    mptr->_11 += ((iptr->_11 - mptr->_11) * m_IMFrac) / m_IMRate;
    mptr->_12 += ((iptr->_12 - mptr->_12) * m_IMFrac) / m_IMRate;
    mptr->_13 += ((iptr->_13 - mptr->_13) * m_IMFrac) / m_IMRate;
    mptr->_20 += ((iptr->_20 - mptr->_20) * m_IMFrac) / m_IMRate;
    mptr->_21 += ((iptr->_21 - mptr->_21) * m_IMFrac) / m_IMRate;
    mptr->_22 += ((iptr->_22 - mptr->_22) * m_IMFrac) / m_IMRate;
    mptr->_23 += ((iptr->_23 - mptr->_23) * m_IMFrac) / m_IMRate;
}

void Matrix_InterpolateArm(void)
{
    MATRIX *mptr = g_MatrixPtr;
    MATRIX *iptr = m_IMMatrixPtr;

    mptr->_00 = mptr[-2]._00;
    mptr->_01 = mptr[-2]._01;
    mptr->_02 = mptr[-2]._02;
    mptr->_03 += ((iptr->_03 - mptr->_03) * m_IMFrac) / m_IMRate;
    mptr->_10 = mptr[-2]._10;
    mptr->_11 = mptr[-2]._11;
    mptr->_12 = mptr[-2]._12;
    mptr->_13 += ((iptr->_13 - mptr->_13) * m_IMFrac) / m_IMRate;
    mptr->_20 = mptr[-2]._20;
    mptr->_21 = mptr[-2]._21;
    mptr->_22 = mptr[-2]._22;
    mptr->_23 += ((iptr->_23 - mptr->_23) * m_IMFrac) / m_IMRate;
}

void Matrix_Push_I(void)
{
    Matrix_Push();
    m_IMMatrixPtr[1]._00 = m_IMMatrixPtr[0]._00;
    m_IMMatrixPtr[1]._01 = m_IMMatrixPtr[0]._01;
    m_IMMatrixPtr[1]._02 = m_IMMatrixPtr[0]._02;
    m_IMMatrixPtr[1]._03 = m_IMMatrixPtr[0]._03;
    m_IMMatrixPtr[1]._10 = m_IMMatrixPtr[0]._10;
    m_IMMatrixPtr[1]._11 = m_IMMatrixPtr[0]._11;
    m_IMMatrixPtr[1]._12 = m_IMMatrixPtr[0]._12;
    m_IMMatrixPtr[1]._13 = m_IMMatrixPtr[0]._13;
    m_IMMatrixPtr[1]._20 = m_IMMatrixPtr[0]._20;
    m_IMMatrixPtr[1]._21 = m_IMMatrixPtr[0]._21;
    m_IMMatrixPtr[1]._22 = m_IMMatrixPtr[0]._22;
    m_IMMatrixPtr[1]._23 = m_IMMatrixPtr[0]._23;
    m_IMMatrixPtr++;
}

void Matrix_Pop_I(void)
{
    Matrix_Pop();
    m_IMMatrixPtr--;
}

void Matrix_TranslateRel_I(int32_t x, int32_t y, int32_t z)
{
    Matrix_TranslateRel(x, y, z);
    MATRIX *old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_TranslateRel(x, y, z);
    g_MatrixPtr = old_matrix;
}

void Matrix_TranslateRel_ID(
    int32_t x, int32_t y, int32_t z, int32_t x2, int32_t y2, int32_t z2)
{
    Matrix_TranslateRel(x, y, z);
    MATRIX *old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_TranslateRel(x2, y2, z2);
    g_MatrixPtr = old_matrix;
}

void Matrix_RotY_I(PHD_ANGLE ang)
{
    Matrix_RotY(ang);
    MATRIX *old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_RotY(ang);
    g_MatrixPtr = old_matrix;
}

void Matrix_RotX_I(PHD_ANGLE ang)
{
    Matrix_RotX(ang);
    MATRIX *old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_RotX(ang);
    g_MatrixPtr = old_matrix;
}

void Matrix_RotZ_I(PHD_ANGLE ang)
{
    Matrix_RotZ(ang);
    MATRIX *old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_RotZ(ang);
    g_MatrixPtr = old_matrix;
}

void Matrix_RotYXZ_I(PHD_ANGLE y, PHD_ANGLE x, PHD_ANGLE z)
{
    Matrix_RotYXZ(y, x, z);
    MATRIX *old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_RotYXZ(y, x, z);
    g_MatrixPtr = old_matrix;
}

void Matrix_RotYXZpack_I(int32_t r1, int32_t r2)
{
    Matrix_RotYXZpack(r1);
    MATRIX *old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_RotYXZpack(r2);
    g_MatrixPtr = old_matrix;
}

void Matrix_LookAt(
    int32_t xsrc, int32_t ysrc, int32_t zsrc, int32_t xtar, int32_t ytar,
    int32_t ztar, int16_t roll)
{
    PHD_ANGLE angles[2];
    Math_GetVectorAngles(xtar - xsrc, ytar - ysrc, ztar - zsrc, angles);

    const XYZ_32 view_pos = {
        .x = xsrc,
        .y = ysrc,
        .z = zsrc,
    };
    const XYZ_16 view_rot = {
        .x = angles[1],
        .y = angles[0],
        .z = roll,
    };
    Matrix_GenerateW2V(&view_pos, &view_rot);
}
