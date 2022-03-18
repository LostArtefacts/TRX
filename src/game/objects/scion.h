#pragma once

#include "global/types.h"

#include <stdint.h>

void Scion_Setup1(OBJECT_INFO *obj);
void Scion_Setup2(OBJECT_INFO *obj);
void Scion_Setup3(OBJECT_INFO *obj);
void Scion_Setup4(OBJECT_INFO *obj);
void Scion_SetupHolder(OBJECT_INFO *obj);
void Scion_Control(int16_t item_num);
void Scion_Control3(int16_t item_num);
void Scion_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void Scion_Collision4(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
