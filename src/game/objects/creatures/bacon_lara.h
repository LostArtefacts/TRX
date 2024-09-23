#pragma once

#include "global/types.h"

void BaconLara_Setup(OBJECT *obj);
void BaconLara_Initialise(int16_t item_num);
bool BaconLara_InitialiseAnchor(int32_t room_index);
void BaconLara_Control(int16_t item_num);
void BaconLara_Draw(ITEM *item);
