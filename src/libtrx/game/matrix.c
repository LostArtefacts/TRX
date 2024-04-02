#include "game/matrix.h"

#include "game/const.h"
#include "game/math.h"
#include "utils.h"

#include <float.h>
#include <math.h>

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

static inline void M_QuaternionNormalize(QUATERNION *q)
{
    const double n2 = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;
    if (n2 > 0.0) {
        const double inv = 1.0 / sqrt(n2);
        q->x *= inv;
        q->y *= inv;
        q->z *= inv;
        q->w *= inv;
    } else {
        // fallback: identity
        q->x = q->y = q->z = 0.0;
        q->w = 1.0;
    }
}

// One inexpensive polar-decomposition iteration to orthonormalize R (3x3).
// R <- R * (3I - R^T R) / 2  (Newton step toward orthogonal)
// Works great if R is already close to rotation.
static void M_Double3x3Ortho(double r[3][3])
{
    double rt_r[3][3] = {};
    for (int32_t i = 0; i < 3; i++) {
        for (int32_t j = 0; j < 3; j++) {
            for (int32_t k = 0; k < 3; k++) {
                rt_r[i][j] += r[k][i] * r[k][j];
            }
        }
    }

    double m[3][3];
    for (int32_t i = 0; i < 3; i++) {
        for (int32_t j = 0; j < 3; j++) {
            m[i][j] = 3.0 * (i == j) - rt_r[i][j];
        }
    }

    double rn[3][3] = {};
    for (int32_t i = 0; i < 3; i++) {
        for (int32_t j = 0; j < 3; j++) {
            for (int32_t k = 0; k < 3; k++) {
                rn[i][j] += r[i][k] * m[k][j];
            }
        }
    }

    // divide by 2
    for (int32_t i = 0; i < 3; i++) {
        for (int32_t j = 0; j < 3; j++) {
            r[i][j] = 0.5 * rn[i][j];
        }
    }
}

// Extract 3x3 rotation (in doubles) from fixed-point MATRIX.
static void M_Double3x3FromMatrix(const MATRIX *const m, double e[3][3])
{
    const double s = (1 << W2V_SHIFT);
    e[0][0] = m->_00 / s;
    e[0][1] = m->_01 / s;
    e[0][2] = m->_02 / s;
    e[1][0] = m->_10 / s;
    e[1][1] = m->_11 / s;
    e[1][2] = m->_12 / s;
    e[2][0] = m->_20 / s;
    e[2][1] = m->_21 / s;
    e[2][2] = m->_22 / s;
}

// Remove uniform scale if present (estimate from row lengths).
// Use average row length to reduce noise.
static void M_Double3x3RemoveScale(double e[3][3])
{
    const double rl0 =
        sqrt(e[0][0] * e[0][0] + e[0][1] * e[0][1] + e[0][2] * e[0][2]);
    const double rl1 =
        sqrt(e[1][0] * e[1][0] + e[1][1] * e[1][1] + e[1][2] * e[1][2]);
    const double rl2 =
        sqrt(e[2][0] * e[2][0] + e[2][1] * e[2][1] + e[2][2] * e[2][2]);
    const double scale = (rl0 + rl1 + rl2) / 3.0;
    if (scale <= 0.0) {
        return;
    }
    const double inv = 1.0 / scale;
    for (int32_t i = 0; i < 3; i++) {
        for (int32_t j = 0; j < 3; j++) {
            e[i][j] *= inv;
        }
    }
}

// Write 3x3 back to MATRIX as fixed-point, with rounding.
static void M_Double3x3ToMatrix(const double e[3][3], MATRIX *m)
{
    const double s = (double)(1 << W2V_SHIFT);
    m->_00 = (int32_t)llround(e[0][0] * s);
    m->_01 = (int32_t)llround(e[0][1] * s);
    m->_02 = (int32_t)llround(e[0][2] * s);
    m->_10 = (int32_t)llround(e[1][0] * s);
    m->_11 = (int32_t)llround(e[1][1] * s);
    m->_12 = (int32_t)llround(e[1][2] * s);
    m->_20 = (int32_t)llround(e[2][0] * s);
    m->_21 = (int32_t)llround(e[2][1] * s);
    m->_22 = (int32_t)llround(e[2][2] * s);
}

static void M_MatrixToQuaternion(const MATRIX *m, QUATERNION *q)
{
    double e[3][3];
    M_Double3x3FromMatrix(m, e);
    M_Double3x3RemoveScale(e);
    // Orthonormalize (fast, one Newton step is usually enough).
    M_Double3x3Ortho(e);

    const double tr = e[0][0] + e[1][1] + e[2][2];
    if (tr > 0.0) {
        const double s = sqrt(tr + 1.0) * 2.0; // 4*w
        q->w = 0.25 * s;
        q->x = (e[2][1] - e[1][2]) / s;
        q->y = (e[0][2] - e[2][0]) / s;
        q->z = (e[1][0] - e[0][1]) / s;
    } else {
        // Pick the biggest diagonal for numerical stability
        int32_t i = 0;
        if (e[1][1] > e[0][0]) {
            i = 1;
        }
        if (e[2][2] > e[i][i]) {
            i = 2;
        }
        const int32_t j = (i + 1) % 3;
        const int32_t k = (i + 2) % 3;
        const double s = sqrt(e[i][i] - e[j][j] - e[k][k] + 1.0) * 2.0;
        double qv[3];
        qv[i] = 0.25 * s;
        qv[j] = (e[j][i] + e[i][j]) / s;
        qv[k] = (e[k][i] + e[i][k]) / s;
        q->x = qv[0];
        q->y = qv[1];
        q->z = qv[2];
        q->w = (e[k][j] - e[j][k]) / s;
    }
    M_QuaternionNormalize(q);
}

static void M_MatrixFromQuaternion(const QUATERNION *const qin, MATRIX *const m)
{
    QUATERNION q = *qin;
    M_QuaternionNormalize(&q);

    const double xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const double xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const double wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    double e[3][3];
    e[0][0] = 1.0 - 2.0 * (yy + zz);
    e[0][1] = 2.0 * (xy - wz);
    e[0][2] = 2.0 * (xz + wy);
    e[1][0] = 2.0 * (xy + wz);
    e[1][1] = 1.0 - 2.0 * (xx + zz);
    e[1][2] = 2.0 * (yz - wx);
    e[2][0] = 2.0 * (xz - wy);
    e[2][1] = 2.0 * (yz + wx);
    e[2][2] = 1.0 - 2.0 * (xx + yy);

    // Optional: one more orthonormalization step to crush rounding noise
    M_Double3x3Ortho(e);
    M_Double3x3ToMatrix(e, m);
}

static void M_QuaternionSlerp(
    const QUATERNION *const qa, const QUATERNION *const qb, const double t,
    QUATERNION *const out)
{
    QUATERNION a = *qa, b = *qb;
    M_QuaternionNormalize(&a);
    M_QuaternionNormalize(&b);

    double cosom = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (cosom < 0.0) { // take shortest path
        cosom = -cosom;
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
    }

    // Guard acos input, and use nlerp for tiny angles
    CLAMP(cosom, -1.0, 1.0);

    // Threshold tuned for double precision
    const double EPS = 1e-12;
    double scale0;
    double scale1;
    if (1.0 - cosom > EPS) {
        const double omega = acos(cosom);
        const double sinom = 1.0 / sin(omega);
        scale0 = sin((1.0 - t) * omega) * sinom;
        scale1 = sin(t * omega) * sinom;
    } else {
        // Nearly parallel: nlerp, then normalize
        scale0 = 1.0 - t;
        scale1 = t;
    }

    out->x = scale0 * a.x + scale1 * b.x;
    out->y = scale0 * a.y + scale1 * b.y;
    out->z = scale0 * a.z + scale1 * b.z;
    out->w = scale0 * a.w + scale1 * b.w;
    M_QuaternionNormalize(out);
}

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

// ----- your high-level blend -----

void Matrix_Interpolate(void)
{
    MATRIX *const m1 = g_MatrixPtr;
    const MATRIX *const m2 = m_IMMatrixPtr;
    MATRIX *result = g_MatrixPtr;

    // Robust, bounded rate
    double rate = (m_IMRate != 0) ? ((double)m_IMFrac / (double)m_IMRate) : 0.0;
    CLAMP(rate, 0.0, 1.0);

    QUATERNION q1, q2, q;
    M_MatrixToQuaternion(m1, &q1);
    M_MatrixToQuaternion(m2, &q2);
    M_QuaternionSlerp(&q1, &q2, rate, &q);
    M_MatrixFromQuaternion(&q, result);

    // Linearly blend translation (kept as-is; if your matrices carry scale in
    // _03.._23 you’d strip it too)
    result->_03 = (int32_t)llround(m1->_03 + (m2->_03 - m1->_03) * rate);
    result->_13 = (int32_t)llround(m1->_13 + (m2->_13 - m1->_13) * rate);
    result->_23 = (int32_t)llround(m1->_23 + (m2->_23 - m1->_23) * rate);
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
