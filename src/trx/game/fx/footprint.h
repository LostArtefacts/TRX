#pragma once

#include <trx/core/math/types.h>
#include <trx/game/items/types.h>

#include <stdint.h>

typedef struct {
    XYZ_32 pos;
    int16_t room_num;
    int16_t y_rot;
    int16_t life;
} FX_FOOTPRINT;

void FX_Footprint_Reset(void);

void FX_Footprint_Add(const ITEM *lara_item, bool is_left_foot);

void FX_Footprint_Control(void);
void FX_Footprint_Draw(void);

bool FX_Footprint_HasActivePrints(void);
FX_FOOTPRINT *FX_Footprint_GetPrint(int32_t idx);
