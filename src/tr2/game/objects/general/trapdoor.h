#pragma once

#include "global/types.h"

int32_t __cdecl Trapdoor_IsItemOnTop(const ITEM *item, int32_t x, int32_t z);

void __cdecl Trapdoor_Floor(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int32_t *out_height);
void __cdecl Trapdoor_Ceiling(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int32_t *out_height);
void __cdecl Trapdoor_Control(int16_t item_num);
