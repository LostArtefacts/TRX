#pragma once

#include "global/types.h"

#define TRIGMULT2(A, B) (((A) * (B)) >> W2V_SHIFT)
#define TRIGMULT3(A, B, C) (TRIGMULT2((TRIGMULT2(A, B)), C))

void phd_ResetMatrixStack(void);
void phd_GenerateW2V(PHD_3DPOS *viewpos);

void phd_PushMatrix(void);
void phd_PushUnitMatrix(void);
void phd_PopMatrix(void);

void phd_RotX(PHD_ANGLE rx);
void phd_RotY(PHD_ANGLE ry);
void phd_RotZ(PHD_ANGLE rz);
void phd_RotYXZ(PHD_ANGLE ry, PHD_ANGLE rx, PHD_ANGLE rz);
void phd_RotYXZpack(int32_t rots);
void phd_TranslateRel(int32_t x, int32_t y, int32_t z);
void phd_TranslateAbs(int32_t x, int32_t y, int32_t z);

void phd_PushMatrix_I(void);
void phd_PopMatrix_I(void);

void phd_RotY_I(int16_t ang);
void phd_RotX_I(int16_t ang);
void phd_RotZ_I(int16_t ang);
void phd_RotYXZ_I(int16_t y, int16_t x, int16_t z);
void phd_RotYXZpack_I(int32_t r1, int32_t r2);

void phd_TranslateRel_I(int32_t x, int32_t y, int32_t z);
void phd_TranslateRel_ID(
    int32_t x, int32_t y, int32_t z, int32_t x2, int32_t y2, int32_t z2);

void InitInterpolate(int32_t frac, int32_t rate);
void InterpolateMatrix(void);
void InterpolateArmMatrix(void);
