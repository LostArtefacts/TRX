#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Lava_TestFloor(ITEM_INFO *item);
void Lava_Burn(ITEM_INFO *item);

void Lava_Setup(OBJECT_INFO *obj);
void Lava_Control(int16_t fx_num);

void LavaEmitter_Setup(OBJECT_INFO *obj);
void LavaEmitter_Control(int16_t item_num);

void LavaWedge_Setup(OBJECT_INFO *obj);
void LavaWedge_Control(int16_t item_num);
