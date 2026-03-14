#pragma once

#include <trx/core/colors.h>
#include <trx/core/math/types.h>

#include <stdint.h>

typedef struct {
    XZ_16 pos;
    RGB_888 color;
} FX_EXPLOSION_VERT;

typedef struct {
    int16_t on;
    int16_t life;
    int16_t speed;
    int16_t radius;
    XZ_16 rot;
    XYZ_32 pos;
    FX_EXPLOSION_VERT verts[16];
} FX_EXPLOSION_RING;

void FX_ExplosionRing_Reset(void);

void FX_ExplosionRing_Control(void);
void FX_ExplosionRing_Draw(void);
void FX_ExplosionRing_SpawnKnockBack(XYZ_32 pos);
void FX_ExplosionRing_BounceKnockBack(void);
bool FX_ExplosionRing_IsKnockBackActive(void);

FX_EXPLOSION_RING *FX_ExplosionRing_GetRing(int32_t idx);
FX_EXPLOSION_RING *FX_ExplosionRing_GetSummonRing(int32_t idx);
FX_EXPLOSION_RING *FX_ExplosionRing_GetKnockBackRing(int32_t idx);
const FX_EXPLOSION_RING *FX_ExplosionRing_PeekKnockBackRing(int32_t idx);
