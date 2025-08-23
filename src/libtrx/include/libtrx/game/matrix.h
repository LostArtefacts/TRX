#pragma once

#include "./math/types.h"

#define TRIGMULT2(A, B) (((A) * (B)) >> W2V_SHIFT)
#define TRIGMULT3(A, B, C) (TRIGMULT2((TRIGMULT2(A, B)), C))

typedef struct {
    int32_t _00;
    int32_t _01;
    int32_t _02;
    int32_t _03;
    int32_t _10;
    int32_t _11;
    int32_t _12;
    int32_t _13;
    int32_t _20;
    int32_t _21;
    int32_t _22;
    int32_t _23;
} MATRIX;

extern MATRIX *g_MatrixPtr;
extern MATRIX g_W2VMatrix;
extern MATRIX g_IDMatrix;

void Matrix_ResetStack(void);

void Matrix_GenerateW2V(const XYZ_32 *pos, const XYZ_16 *rot);
void Matrix_LookAt(
    int32_t xsrc, int32_t ysrc, int32_t zsrc, int32_t xtar, int32_t ytar,
    int32_t ztar, int16_t roll);

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

void Matrix_TranslateRel(int32_t x, int32_t y, int32_t z);
void Matrix_TranslateRel16(XYZ_16 offset);
void Matrix_TranslateRel32(XYZ_32 offset);
void Matrix_TranslateAbs(int32_t x, int32_t y, int32_t z);
void Matrix_TranslateAbs16(XYZ_16 offset);
void Matrix_TranslateAbs32(XYZ_32 offset);
void Matrix_TranslateSet(int32_t x, int32_t y, int32_t z);

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

void Matrix_InitInterpolate(int32_t frac, int32_t rate);
void Matrix_Interpolate(void);
void Matrix_InterpolateArm(void);
