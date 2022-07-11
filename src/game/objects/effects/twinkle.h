#pragma once

#include "global/types.h"

#include <stdint.h>

void Twinkle_Setup(OBJECT_INFO *obj);
void Twinkle_Control(int16_t fx_num);
void Twinkle_Spawn(GAME_VECTOR *pos);
void Twinkle_SparkleItem(ITEM_INFO *item, int mesh_mask);
