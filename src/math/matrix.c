#include "math/matrix.h"

#include "global/vars.h"
#include "math/math.h"
#include "math/math_misc.h"

#include <math.h>
#include <stddef.h>
#include <float.h>

#define EXTRACT_ROT_Y(rots) (((rots >> 10) & 0x3FF) << 6)
#define EXTRACT_ROT_X(rots) (((rots >> 20) & 0x3FF) << 6)
#define EXTRACT_ROT_Z(rots) ((rots & 0x3FF) << 6)

static MATRIX m_MatrixStack[MAX_MATRICES] = { 0 };
static int32_t m_IMRate = 0;
static int32_t m_IMFrac = 0;
static MATRIX *m_IMMatrixPtr = NULL;
static MATRIX m_IMMatrixStack[MAX_NESTED_MATRICES] = { 0 };

static void Matrix_ToQuaternion(const MATRIX *const m, QUATERNION *result);
static void Matrix_FromQuaternion(const QUATERNION *const q, MATRIX *result);
static void Quaternion_Slerp(
    const QUATERNION *const q1, const QUATERNION *const q2, const float rate,
    QUATERNION *result);

static void Matrix_ToQuaternion(const MATRIX *const m, QUATERNION *result)
{
    const float e00 = m->_00 / (float)W2V_SCALE;
    const float e01 = m->_01 / (float)W2V_SCALE;
    const float e02 = m->_02 / (float)W2V_SCALE;
    const float e10 = m->_10 / (float)W2V_SCALE;
    const float e11 = m->_11 / (float)W2V_SCALE;
    const float e12 = m->_12 / (float)W2V_SCALE;
    const float e20 = m->_20 / (float)W2V_SCALE;
    const float e21 = m->_21 / (float)W2V_SCALE;
    const float e22 = m->_22 / (float)W2V_SCALE;

    const float t = 1.0f + e00 + e11 + e22;
    if (t > 0.0001f) {
        const float s = 0.5f / sqrtf(t);
        result->x = (e21 - e12) * s;
        result->y = (e02 - e20) * s;
        result->z = (e10 - e01) * s;
        result->w = 0.25f / s;
    } else if (e00 > e11 && e00 > e22) {
        const float s = 0.5f / sqrtf(1.0f + e00 - e11 - e22);
        result->x = 0.25f / s;
        result->y = (e01 + e10) * s;
        result->z = (e02 + e20) * s;
        result->w = (e21 - e12) * s;
    } else if (e11 > e22) {
        const float s = 0.5f / sqrtf(1.0f - e00 + e11 - e22);
        result->x = (e01 + e10) * s;
        result->y = 0.25f / s;
        result->z = (e12 + e21) * s;
        result->w = (e02 - e20) * s;
    } else {
        const float s = 0.5f / sqrtf(1.0f - e00 - e11 + e22);
        result->x = (e02 + e20) * s;
        result->y = (e12 + e21) * s;
        result->z = 0.25f / s;
        result->w = (e10 - e01) * s;
    }
}

static void Matrix_FromQuaternion(const QUATERNION *const q, MATRIX *result)
{
    const float sx = q->x * q->x;
    const float sy = q->y * q->y;
    const float sz = q->z * q->z;
    const float sw = q->w * q->w;
    float inv = 1.0f / (sx + sy + sz + sw);

    const float e00 = (sx - sy - sz + sw) * inv;
    const float e11 = (-sx + sy - sz + sw) * inv;
    const float e22 = (-sx - sy + sz + sw) * inv;
    inv *= 2.0f;

    float t1 = q->x * q->y;
    float t2 = q->z * q->w;
    const float e10 = (t1 + t2) * inv;
    const float e01 = (t1 - t2) * inv;

    t1 = q->x * q->z;
    t2 = q->y * q->w;
    const float e20 = (t1 - t2) * inv;
    const float e02 = (t1 + t2) * inv;

    t1 = q->y * q->z;
    t2 = q->x * q->w;
    const float e21 = (t1 + t2) * inv;
    const float e12 = (t1 - t2) * inv;

    result->_00 = e00 * W2V_SCALE;
    result->_01 = e01 * W2V_SCALE;
    result->_02 = e02 * W2V_SCALE;
    result->_10 = e10 * W2V_SCALE;
    result->_11 = e11 * W2V_SCALE;
    result->_12 = e12 * W2V_SCALE;
    result->_20 = e20 * W2V_SCALE;
    result->_21 = e21 * W2V_SCALE;
    result->_22 = e22 * W2V_SCALE;
}

static void Quaternion_Slerp(
    const QUATERNION *const q1, const QUATERNION *const q2, const float rate,
    QUATERNION *result)
{
    if (rate <= 0.0f) {
        *result = *q1;
        return;
    }
    if (rate >= 1.0f) {
        *result = *q2;
        return;
    }

    QUATERNION temp;
    float cosom = q1->x * q2->x + q1->y * q2->y + q1->z * q2->z + q1->w * q2->w;
    if (cosom < 0.0f) {
        temp.x = -q2->x;
        temp.y = -q2->y;
        temp.z = -q2->z;
        temp.w = -q2->w;
        cosom = -cosom;
    } else
        temp = *q2;

    float scale0;
    float scale1;
    if (1.0f - cosom > FLT_EPSILON) {
        const float omega = acosf(cosom);
        const float sinom = 1.0f / sinf(omega);
        scale0 = sinf((1.0f - rate) * omega) * sinom;
        scale1 = sinf(rate * omega) * sinom;
    } else {
        scale0 = 1.0f - rate;
        scale1 = rate;
    }

    result->x = q1->x * scale0 + temp.x * scale1;
    result->y = q1->y * scale0 + temp.y * scale1;
    result->z = q1->z * scale0 + temp.z * scale1;
    result->w = q1->w * scale0 + temp.w * scale1;
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

void Matrix_RotY(PHD_ANGLE ry)
{
    if (!ry) {
        return;
    }

    MATRIX *mptr = g_MatrixPtr;
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

void Matrix_RotZ(PHD_ANGLE rz)
{
    if (!rz) {
        return;
    }

    MATRIX *mptr = g_MatrixPtr;
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

void Matrix_RotYXZ(PHD_ANGLE ry, PHD_ANGLE rx, PHD_ANGLE rz)
{
    Matrix_RotY(ry);
    Matrix_RotX(rx);
    Matrix_RotZ(rz);
}

void Matrix_RotYXZpack(int32_t rots)
{
    const PHD_ANGLE ry = EXTRACT_ROT_Y(rots);
    const PHD_ANGLE rx = EXTRACT_ROT_X(rots);
    const PHD_ANGLE rz = EXTRACT_ROT_Z(rots);
    Matrix_RotY(ry);
    Matrix_RotX(rx);
    Matrix_RotZ(rz);
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
    MATRIX *m1 = g_MatrixPtr;
    MATRIX *m2 = m_IMMatrixPtr;
    MATRIX *result = g_MatrixPtr;
    const double rate = m_IMFrac / (double)m_IMRate;

    QUATERNION q1, q2, q;
    Matrix_ToQuaternion(m1, &q1);
    Matrix_ToQuaternion(m2, &q2);

    Quaternion_Slerp(&q1, &q2, rate, &q);

    Matrix_FromQuaternion(&q, result);
    result->_03 = m1->_03 + (m2->_03 - m1->_03) * rate;
    result->_13 = m1->_13 + (m2->_13 - m1->_13) * rate;
    result->_23 = m1->_23 + (m2->_23 - m1->_23) * rate;
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
