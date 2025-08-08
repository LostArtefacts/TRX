#pragma once

#include "../math.h"
#include "../types.h"

#include <stdint.h>

void Walkable_Add(int16_t item_num, XYZ_32 pos);
void Walkable_Remove(int16_t item_num);
void Walkable_Reset(void);
void Walkable_Reposition(
    int16_t item_num, GAME_VECTOR start, GAME_VECTOR target);
