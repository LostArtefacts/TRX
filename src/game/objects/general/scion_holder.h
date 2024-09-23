#pragma once

#include "global/types.h"

void Scion2_Setup(OBJECT *obj);
void Scion3_Setup(OBJECT *obj);
void Scion4_Setup(OBJECT *obj);
void Scion4_Control(int16_t item_num);
void ScionHolder_Setup(OBJECT *obj);
void ScionHolder_Control(int16_t item_num);
void Scion3_Control(int16_t item_num);
void Scion4_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
