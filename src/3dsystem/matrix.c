#include "3dsystem/matrix.h"

#include "3dsystem/phd_math.h"
#include "global/vars.h"

#define EXTRACT_ROT_Y(rots) (((rots >> 10) & 0x3FF) << 6)
#define EXTRACT_ROT_X(rots) (((rots >> 20) & 0x3FF) << 6)
#define EXTRACT_ROT_Z(rots) ((rots & 0x3FF) << 6)

static PHD_MATRIX m_MatrixStack[MAX_MATRICES] = { 0 };
static int32_t m_IMRate = 0;
static int32_t m_IMFrac = 0;
static PHD_MATRIX *m_IMMatrixPtr = NULL;
static PHD_MATRIX m_IMMatrixStack[MAX_NESTED_MATRICES] = { 0 };

void phd_ResetMatrixStack(void)
{
    g_PhdMatrixPtr = &m_MatrixStack[0];
}

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

void phd_PushMatrix(void)
{
    g_PhdMatrixPtr++;
    g_PhdMatrixPtr[0] = g_PhdMatrixPtr[-1];
}

void phd_PushUnitMatrix(void)
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

void phd_PopMatrix(void)
{
    g_PhdMatrixPtr--;
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

void phd_TranslateRel(int32_t x, int32_t y, int32_t z)
{
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    mptr->_03 += mptr->_00 * x + mptr->_01 * y + mptr->_02 * z;
    mptr->_13 += mptr->_10 * x + mptr->_11 * y + mptr->_12 * z;
    mptr->_23 += mptr->_20 * x + mptr->_21 * y + mptr->_22 * z;
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

void InitInterpolate(int32_t frac, int32_t rate)
{
    m_IMFrac = frac;
    m_IMRate = rate;
    m_IMMatrixPtr = &m_IMMatrixStack[0];
    m_IMMatrixPtr->_00 = g_PhdMatrixPtr->_00;
    m_IMMatrixPtr->_01 = g_PhdMatrixPtr->_01;
    m_IMMatrixPtr->_02 = g_PhdMatrixPtr->_02;
    m_IMMatrixPtr->_03 = g_PhdMatrixPtr->_03;
    m_IMMatrixPtr->_10 = g_PhdMatrixPtr->_10;
    m_IMMatrixPtr->_11 = g_PhdMatrixPtr->_11;
    m_IMMatrixPtr->_12 = g_PhdMatrixPtr->_12;
    m_IMMatrixPtr->_13 = g_PhdMatrixPtr->_13;
    m_IMMatrixPtr->_20 = g_PhdMatrixPtr->_20;
    m_IMMatrixPtr->_21 = g_PhdMatrixPtr->_21;
    m_IMMatrixPtr->_22 = g_PhdMatrixPtr->_22;
    m_IMMatrixPtr->_23 = g_PhdMatrixPtr->_23;
}

void InterpolateMatrix(void)
{
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    PHD_MATRIX *iptr = m_IMMatrixPtr;

    if (m_IMRate == 2) {
        mptr->_00 = (mptr->_00 + iptr->_00) / 2;
        mptr->_01 = (mptr->_01 + iptr->_01) / 2;
        mptr->_02 = (mptr->_02 + iptr->_02) / 2;
        mptr->_03 = (mptr->_03 + iptr->_03) / 2;
        mptr->_10 = (mptr->_10 + iptr->_10) / 2;
        mptr->_11 = (mptr->_11 + iptr->_11) / 2;
        mptr->_12 = (mptr->_12 + iptr->_12) / 2;
        mptr->_13 = (mptr->_13 + iptr->_13) / 2;
        mptr->_20 = (mptr->_20 + iptr->_20) / 2;
        mptr->_21 = (mptr->_21 + iptr->_21) / 2;
        mptr->_22 = (mptr->_22 + iptr->_22) / 2;
        mptr->_23 = (mptr->_23 + iptr->_23) / 2;
    } else {
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
}

void InterpolateArmMatrix(void)
{
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    PHD_MATRIX *iptr = m_IMMatrixPtr;

    if (m_IMRate == 2) {
        mptr->_00 = mptr[-2]._00;
        mptr->_01 = mptr[-2]._01;
        mptr->_02 = mptr[-2]._02;
        mptr->_03 = (mptr->_03 + iptr->_03) / 2;
        mptr->_10 = mptr[-2]._10;
        mptr->_11 = mptr[-2]._11;
        mptr->_12 = mptr[-2]._12;
        mptr->_13 = (mptr->_13 + iptr->_13) / 2;
        mptr->_20 = mptr[-2]._20;
        mptr->_21 = mptr[-2]._21;
        mptr->_22 = mptr[-2]._22;
        mptr->_23 = (mptr->_23 + iptr->_23) / 2;
    } else {
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
}

void phd_PushMatrix_I(void)
{
    phd_PushMatrix();
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

void phd_PopMatrix_I(void)
{
    phd_PopMatrix();
    m_IMMatrixPtr--;
}

void phd_TranslateRel_I(int32_t x, int32_t y, int32_t z)
{
    phd_TranslateRel(x, y, z);
    PHD_MATRIX *old_matrix = g_PhdMatrixPtr;
    g_PhdMatrixPtr = m_IMMatrixPtr;
    phd_TranslateRel(x, y, z);
    g_PhdMatrixPtr = old_matrix;
}

void phd_TranslateRel_ID(
    int32_t x, int32_t y, int32_t z, int32_t x2, int32_t y2, int32_t z2)
{
    phd_TranslateRel(x, y, z);
    PHD_MATRIX *old_matrix = g_PhdMatrixPtr;
    g_PhdMatrixPtr = m_IMMatrixPtr;
    phd_TranslateRel(x2, y2, z2);
    g_PhdMatrixPtr = old_matrix;
}

void phd_RotY_I(PHD_ANGLE ang)
{
    phd_RotY(ang);
    PHD_MATRIX *old_matrix = g_PhdMatrixPtr;
    g_PhdMatrixPtr = m_IMMatrixPtr;
    phd_RotY(ang);
    g_PhdMatrixPtr = old_matrix;
}

void phd_RotX_I(PHD_ANGLE ang)
{
    phd_RotX(ang);
    PHD_MATRIX *old_matrix = g_PhdMatrixPtr;
    g_PhdMatrixPtr = m_IMMatrixPtr;
    phd_RotX(ang);
    g_PhdMatrixPtr = old_matrix;
}

void phd_RotZ_I(PHD_ANGLE ang)
{
    phd_RotZ(ang);
    PHD_MATRIX *old_matrix = g_PhdMatrixPtr;
    g_PhdMatrixPtr = m_IMMatrixPtr;
    phd_RotZ(ang);
    g_PhdMatrixPtr = old_matrix;
}

void phd_RotYXZ_I(PHD_ANGLE y, PHD_ANGLE x, PHD_ANGLE z)
{
    phd_RotYXZ(y, x, z);
    PHD_MATRIX *old_matrix = g_PhdMatrixPtr;
    g_PhdMatrixPtr = m_IMMatrixPtr;
    phd_RotYXZ(y, x, z);
    g_PhdMatrixPtr = old_matrix;
}

void phd_RotYXZpack_I(int32_t r1, int32_t r2)
{
    phd_RotYXZpack(r1);
    PHD_MATRIX *old_matrix = g_PhdMatrixPtr;
    g_PhdMatrixPtr = m_IMMatrixPtr;
    phd_RotYXZpack(r2);
    g_PhdMatrixPtr = old_matrix;
}
