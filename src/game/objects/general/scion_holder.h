#pragma once

#include "global/types.h"

void Scion2_Setup(OBJECT_INFO *obj);
void Scion3_Setup(OBJECT_INFO *obj);
void Scion4_Setup(OBJECT_INFO *obj);
void Scion4_Control(int16_t item_num);
void ScionHolder_Setup(OBJECT_INFO *obj);
void ScionHolder_Control(int16_t item_num);
void Scion3_Control(int16_t item_num);
void Scion4_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
