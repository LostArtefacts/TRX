#pragma once

#include <trx/game/items/types.h>

typedef struct {
    XYZ_32 pos[2];
    XZ_32 vel[2];
    uint8_t life;
} WAKE_FX_POINT;

void WakeFX_ClearPoints(void);
void WakeFX_Update(void);

WAKE_FX_POINT *WakeFX_GetPoint(int32_t wake_idx, int32_t side);

uint8_t WakeFX_GetShade(void);
void WakeFX_SetShade(uint8_t shade);

uint8_t WakeFX_GetStartIndex(void);
void WakeFX_AdvanceStartIndex(void);

void WakeFX_Draw(const ITEM *item);
