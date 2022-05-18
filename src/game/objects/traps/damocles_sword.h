#pragma once

#include "global/types.h"

#include <stdint.h>

void DamoclesSword_Setup(OBJECT_INFO *obj);
void DamoclesSword_Initialise(int16_t item_num);
void DamoclesSword_Control(int16_t item_num);
void DamoclesSword_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
