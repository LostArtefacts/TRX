#pragma once

#include "global/types.h"

int32_t Trapdoor_IsItemOnTop(const ITEM *item, int32_t x, int32_t z);

void Trapdoor_Setup(OBJECT *obj);
void Trapdoor_Control(int16_t item_num);
