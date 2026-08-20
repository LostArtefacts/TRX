#pragma once

#include <trx/core/math/types.h>

typedef struct QUATERNION {
    double x;
    double y;
    double z;
    double w;
} QUATERNION;

typedef struct {
    int64_t _00, _01, _02, _03, _10, _11, _12, _13, _20, _21, _22, _23;
} MATRIX;

// Transforms are held on two parallel stacks: the view stack that g_MatrixPtr
// addresses, and the world stack that g_WMatrixPtr addresses. An operation
// with no suffix applies to both, so that the two stay in step.
//
// Three suffixes vary that:
//
// _M   applies to the matrix passed in and leaves both stacks alone.
// _I   applies to the two stacks and to the interpolation stacks as well,
//      with the same value everywhere.
// _ID  applies the first value to the two stacks and the second value to the
//      interpolation stacks, which is how the two ends of a blend are given
//      different rotations or offsets at the same joint.
//
// The interpolation stacks carry the far end of a blend. Matrix_InitInterpolate
// copies the current matrices into them and sets the weight, the transform then
// proceeds in _I and _ID operations, and Matrix_Interpolate collapses the two
// ends into the current matrices. Push and pop the interpolation stacks with
// Matrix_Push_I and Matrix_Pop_I; they are shallower than the ordinary stacks,
// so a blend nests less deeply than a plain transform.

extern MATRIX *g_MatrixPtr;
extern MATRIX *g_WMatrixPtr;
extern XYZ_32 g_ViewPos;
extern MATRIX g_ViewMatrix;
extern MATRIX g_IDMatrix;

void Matrix_ResetStack(void);

void Matrix_GenerateW2V(const XYZ_32 *pos, const XYZ_16 *rot);
void Matrix_LookAt(
    int32_t xsrc, int32_t ysrc, int32_t zsrc, int32_t xtar, int32_t ytar,
    int32_t ztar, int16_t roll);
void Matrix_ScaleW2V(XYZ_32 scale);

bool Matrix_Push(void);
bool Matrix_PushUnit(void);
void Matrix_Pop(void);

void Matrix_Scale(int32_t scale);
void Matrix_ScaleX(int32_t sx);
void Matrix_ScaleY(int32_t sy);
void Matrix_ScaleZ(int32_t sz);

void Matrix_RotX(int16_t rx);
void Matrix_RotY(int16_t ry);
void Matrix_RotZ(int16_t rz);
void Matrix_Rot16(XYZ_16 rotation);
void Matrix_RotX_M(MATRIX *m, int16_t rx);
void Matrix_RotY_M(MATRIX *m, int16_t ry);
void Matrix_RotZ_M(MATRIX *m, int16_t rz);
void Matrix_Mul3x3_M(MATRIX *out, const MATRIX *lhs, const MATRIX *rhs);
void Matrix_Slerp3x3_M(MATRIX *lhs_out, const MATRIX *rhs, double t);

// Interpolates between two rotations along the arc between them, and gives the
// result back as angles Matrix_Rot16 takes. Interpolating the three angles
// apart from each other instead does not follow that arc: near a quarter turn
// on X, two triples half a turn apart on Y and Z can name the same rotation,
// and meeting them in the middle points the other way.
XYZ_16 Matrix_SlerpRot16(XYZ_16 rotation_1, XYZ_16 rotation_2, double t);

void Matrix_Mul3x3(const MATRIX *rhs);

void Matrix_TranslateRel(int32_t x, int32_t y, int32_t z);
void Matrix_TranslateRel16(XYZ_16 offset);
void Matrix_TranslateRel32(XYZ_32 offset);
void Matrix_TranslateAbs(int32_t x, int32_t y, int32_t z);
void Matrix_TranslateAbs16(XYZ_16 offset);
void Matrix_TranslateAbs32(XYZ_32 offset);
void Matrix_TranslateSet32(XYZ_32 origin);
void Matrix_TranslateSet32_M(MATRIX *out, XYZ_32 origin);

void Matrix_Push_I(void);
void Matrix_Pop_I(void);

void Matrix_RotY_I(int16_t ang);
void Matrix_RotX_I(int16_t ang);
void Matrix_RotZ_I(int16_t ang);
void Matrix_Rot16_I(const XYZ_16 rotation);
void Matrix_Rot16_ID(XYZ_16 rotation_1, XYZ_16 rotation_2);

void Matrix_TranslateRel_I(int32_t x, int32_t y, int32_t z);
void Matrix_TranslateRel16_I(XYZ_16 offset);
void Matrix_TranslateRel32_I(XYZ_32 offset);
void Matrix_TranslateRel_ID(
    int32_t x, int32_t y, int32_t z, int32_t x2, int32_t y2, int32_t z2);
void Matrix_TranslateRel16_ID(XYZ_16 offset_1, XYZ_16 offset_2);
void Matrix_TranslateRel32_ID(XYZ_32 offset_1, XYZ_32 offset_2);

// Starts a blend, at the weight frac/rate. The interpolation stacks take a
// copy of the current matrices, and the operations that follow reach them
// through the _I and _ID suffixes.
void Matrix_InitInterpolate(int32_t frac, int32_t rate);

// Ends a blend, replacing the current matrices with the result. Rotation
// follows the shortest arc between the two ends and position runs straight,
// both at the weight the blend began with.
void Matrix_Interpolate(void);

// Ends a blend for an arm, taking the position from the blend as
// Matrix_Interpolate does, but the rotation from two entries lower on the
// stack. An arm keeps the orientation the item holds it at, so that a gun
// stays pointed where it is aimed while the frames move under it.
void Matrix_InterpolateArm(void);

XYZ_32 Matrix_MulVec32_M(const MATRIX *m, const XYZ_32 v);
XYZ_32 Matrix_GetOffset_M(const MATRIX *m);
