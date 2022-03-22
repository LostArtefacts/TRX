#pragma once

#include "global/types.h"

#include <stdint.h>

#define LAVA_GLOB_DAMAGE 10
#define LAVA_WEDGE_SPEED 25

bool Lava_TestFloor(ITEM_INFO *item);
void Lava_Burn(ITEM_INFO *item);

void Lava_Setup(OBJECT_INFO *obj);
void Lava_Control(int16_t fx_num);

void LavaEmitter_Setup(OBJECT_INFO *obj);
void LavaEmitter_Control(int16_t item_num);

void LavaWedge_Setup(OBJECT_INFO *obj);
void LavaWedge_Control(int16_t item_num);
