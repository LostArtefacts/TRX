#include "game/matrix.h"

#include "game/const.h"
#include "game/math.h"

#define MAX_MATRICES 40
#define MAX_NESTED_MATRICES 32

static MATRIX m_MatrixStack[MAX_MATRICES] = {};
static int32_t m_IMRate = 0;
static int32_t m_IMFrac = 0;
static MATRIX *m_IMMatrixPtr = nullptr;
static MATRIX m_IMMatrixStack[MAX_NESTED_MATRICES] = {};

MATRIX *g_MatrixPtr = &m_MatrixStack[0];
MATRIX g_W2VMatrix = {};
MATRIX g_IDMatrix = {
    // clang-format off
    ._00 = 1 << W2V_SHIFT, ._01 = 0, ._02 = 0, ._03 = 0,
    ._10 = 0, ._11 = 1 << W2V_SHIFT, ._12 = 0, ._13 = 0,
    ._20 = 0, ._21 = 0, ._22 = 1 << W2V_SHIFT, ._23 = 0,
    // clang-format on
};

static void M_RotYXZ(const int16_t ry, const int16_t rx, const int16_t rz)
{
    Matrix_RotY(ry);
    Matrix_RotX(rx);
    Matrix_RotZ(rz);
}

static void M_RotYXZ_I(const int16_t y, const int16_t x, const int16_t z)
{
    M_RotYXZ(y, x, z);
    MATRIX *const old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    M_RotYXZ(y, x, z);
    g_MatrixPtr = old_matrix;
}

void Matrix_ResetStack(void)
{
    g_MatrixPtr = &m_MatrixStack[0];
}

void Matrix_GenerateW2V(const XYZ_32 *pos, const XYZ_16 *rot)
{
    g_MatrixPtr = &m_MatrixStack[0];
    const int32_t sx = Math_Sin(rot->x);
    const int32_t cx = Math_Cos(rot->x);
    const int32_t sy = Math_Sin(rot->y);
    const int32_t cy = Math_Cos(rot->y);
    const int32_t sz = Math_Sin(rot->z);
    const int32_t cz = Math_Cos(rot->z);

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
    g_MatrixPtr++;
    *g_MatrixPtr = g_IDMatrix;
    return true;
}

void Matrix_Pop(void)
{
    g_MatrixPtr--;
}

void Matrix_Scale(const int32_t scale)
{
    Matrix_ScaleX(scale);
    Matrix_ScaleY(scale);
    Matrix_ScaleZ(scale);
}

void Matrix_ScaleX(const int32_t sx)
{
    MATRIX *const mptr = g_MatrixPtr;
    mptr->_00 = ((int64_t)mptr->_00 * sx) >> W2V_SHIFT;
    mptr->_10 = ((int64_t)mptr->_10 * sx) >> W2V_SHIFT;
    mptr->_20 = ((int64_t)mptr->_20 * sx) >> W2V_SHIFT;
}

void Matrix_ScaleY(const int32_t sy)
{
    MATRIX *const mptr = g_MatrixPtr;
    mptr->_01 = ((int64_t)mptr->_01 * sy) >> W2V_SHIFT;
    mptr->_11 = ((int64_t)mptr->_11 * sy) >> W2V_SHIFT;
    mptr->_21 = ((int64_t)mptr->_21 * sy) >> W2V_SHIFT;
}

void Matrix_ScaleZ(const int32_t sz)
{
    MATRIX *const mptr = g_MatrixPtr;
    mptr->_02 = ((int64_t)mptr->_02 * sz) >> W2V_SHIFT;
    mptr->_12 = ((int64_t)mptr->_12 * sz) >> W2V_SHIFT;
    mptr->_22 = ((int64_t)mptr->_22 * sz) >> W2V_SHIFT;
}

void Matrix_RotX(const int16_t rx)
{
    if (!rx) {
        return;
    }

    MATRIX *const mptr = g_MatrixPtr;
    const int32_t sx = Math_Sin(rx);
    const int32_t cx = Math_Cos(rx);

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

void Matrix_RotY(const int16_t ry)
{
    if (!ry) {
        return;
    }

    MATRIX *const mptr = g_MatrixPtr;
    const int32_t sy = Math_Sin(ry);
    const int32_t cy = Math_Cos(ry);

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

void Matrix_RotZ(const int16_t rz)
{
    if (!rz) {
        return;
    }

    MATRIX *const mptr = g_MatrixPtr;
    const int32_t sz = Math_Sin(rz);
    const int32_t cz = Math_Cos(rz);

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

void Matrix_Rot16(const XYZ_16 rotation)
{
    M_RotYXZ(rotation.y, rotation.x, rotation.z);
}

void Matrix_TranslateRel(const int32_t dx, const int32_t dy, const int32_t dz)
{
    MATRIX *const mptr = g_MatrixPtr;
    mptr->_03 += dx * mptr->_00 + dy * mptr->_01 + dz * mptr->_02;
    mptr->_13 += dx * mptr->_10 + dy * mptr->_11 + dz * mptr->_12;
    mptr->_23 += dx * mptr->_20 + dy * mptr->_21 + dz * mptr->_22;
}

void Matrix_TranslateRel16(const XYZ_16 offset)
{
    Matrix_TranslateRel(offset.x, offset.y, offset.z);
}

void Matrix_TranslateRel32(const XYZ_32 offset)
{
    Matrix_TranslateRel(offset.x, offset.y, offset.z);
}

void Matrix_TranslateAbs(const int32_t x, const int32_t y, const int32_t z)
{
    MATRIX *const mptr = g_MatrixPtr;
    const int32_t dx = x - g_W2VMatrix._03;
    const int32_t dy = y - g_W2VMatrix._13;
    const int32_t dz = z - g_W2VMatrix._23;
    mptr->_03 = dx * mptr->_00 + dy * mptr->_01 + dz * mptr->_02;
    mptr->_13 = dx * mptr->_10 + dy * mptr->_11 + dz * mptr->_12;
    mptr->_23 = dx * mptr->_20 + dy * mptr->_21 + dz * mptr->_22;
}

void Matrix_TranslateAbs16(const XYZ_16 offset)
{
    Matrix_TranslateAbs(offset.x, offset.y, offset.z);
}

void Matrix_TranslateAbs32(const XYZ_32 offset)
{
    Matrix_TranslateAbs(offset.x, offset.y, offset.z);
}

void Matrix_TranslateSet(const int32_t x, const int32_t y, const int32_t z)
{
    MATRIX *const mptr = g_MatrixPtr;
    mptr->_03 = x << W2V_SHIFT;
    mptr->_13 = y << W2V_SHIFT;
    mptr->_23 = z << W2V_SHIFT;
}

void Matrix_InitInterpolate(const int32_t frac, const int32_t rate)
{
    m_IMFrac = frac;
    m_IMRate = rate;
    m_IMMatrixPtr = &m_IMMatrixStack[0];
    *m_IMMatrixPtr = *g_MatrixPtr;
}

void Matrix_Interpolate(void)
{
    MATRIX *const mptr = g_MatrixPtr;
    const MATRIX *const iptr = m_IMMatrixPtr;

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
    MATRIX *const mptr = g_MatrixPtr;
    const MATRIX *const iptr = m_IMMatrixPtr;

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
    m_IMMatrixPtr[1] = m_IMMatrixPtr[0];
    m_IMMatrixPtr++;
}

void Matrix_Pop_I(void)
{
    Matrix_Pop();
    m_IMMatrixPtr--;
}

void Matrix_TranslateRel_I(const int32_t x, const int32_t y, const int32_t z)
{
    Matrix_TranslateRel(x, y, z);
    MATRIX *const old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_TranslateRel(x, y, z);
    g_MatrixPtr = old_matrix;
}

void Matrix_TranslateRel16_I(const XYZ_16 offset)
{
    Matrix_TranslateRel_I(offset.x, offset.y, offset.z);
}

void Matrix_TranslateRel32_I(const XYZ_32 offset)
{
    Matrix_TranslateRel_I(offset.x, offset.y, offset.z);
}

void Matrix_TranslateRel_ID(
    const int32_t x, const int32_t y, const int32_t z, const int32_t x2,
    const int32_t y2, const int32_t z2)
{
    Matrix_TranslateRel(x, y, z);
    MATRIX *const old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_TranslateRel(x2, y2, z2);
    g_MatrixPtr = old_matrix;
}

void Matrix_TranslateRel16_ID(const XYZ_16 offset_1, const XYZ_16 offset_2)
{
    Matrix_TranslateRel_ID(
        offset_1.x, offset_1.y, offset_1.z, offset_2.x, offset_2.y, offset_2.z);
}

void Matrix_TranslateRel32_ID(const XYZ_32 offset_1, const XYZ_32 offset_2)
{
    Matrix_TranslateRel_ID(
        offset_1.x, offset_1.y, offset_1.z, offset_2.x, offset_2.y, offset_2.z);
}

void Matrix_RotY_I(const int16_t ang)
{
    Matrix_RotY(ang);
    MATRIX *const old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_RotY(ang);
    g_MatrixPtr = old_matrix;
}

void Matrix_RotX_I(const int16_t ang)
{
    Matrix_RotX(ang);
    MATRIX *const old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_RotX(ang);
    g_MatrixPtr = old_matrix;
}

void Matrix_RotZ_I(const int16_t ang)
{
    Matrix_RotZ(ang);
    MATRIX *const old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_RotZ(ang);
    g_MatrixPtr = old_matrix;
}

void Matrix_Rot16_I(const XYZ_16 rotation)
{
    M_RotYXZ_I(rotation.y, rotation.x, rotation.z);
}

void Matrix_Rot16_ID(const XYZ_16 rotation_1, const XYZ_16 rotation_2)
{
    Matrix_Rot16(rotation_1);
    MATRIX *const old_matrix = g_MatrixPtr;
    g_MatrixPtr = m_IMMatrixPtr;
    Matrix_Rot16(rotation_2);
    g_MatrixPtr = old_matrix;
}

void Matrix_LookAt(
    const int32_t source_x, const int32_t source_y, const int32_t source_z,
    const int32_t target_x, const int32_t target_y, const int32_t target_z,
    const int16_t roll)
{
    int16_t angles[2];
    Math_GetVectorAngles(
        target_x - source_x, target_y - source_y, target_z - source_z, angles);

    const XYZ_32 view_pos = {
        .x = source_x,
        .y = source_y,
        .z = source_z,
    };
    const XYZ_16 view_rot = {
        .x = angles[1],
        .y = angles[0],
        .z = roll,
    };
    Matrix_GenerateW2V(&view_pos, &view_rot);
}
