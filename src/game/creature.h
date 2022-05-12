#pragma once

#include "global/types.h"

void Creature_Initialise(int16_t item_num);
void Creature_AIInfo(ITEM_INFO *item, AI_INFO *info);
void Creature_Mood(ITEM_INFO *item, AI_INFO *info, bool violent);
