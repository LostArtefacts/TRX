#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void BaconLara_Setup(OBJECT_INFO *obj);
void BaconLara_Initialise(int16_t item_num);
bool BaconLara_InitialiseAnchor(int32_t room_index);
void BaconLara_Control(int16_t item_num);
void BaconLara_Draw(ITEM_INFO *item);
