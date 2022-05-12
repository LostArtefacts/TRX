#pragma once

#include "global/types.h"

void Creature_Initialise(int16_t item_num);
void Creature_AIInfo(ITEM_INFO *item, AI_INFO *info);
void Creature_Mood(ITEM_INFO *item, AI_INFO *info, bool violent);
int16_t Creature_Turn(ITEM_INFO *item, int16_t maximum_turn);
void Creature_Tilt(ITEM_INFO *item, int16_t angle);
void Creature_Head(ITEM_INFO *item, int16_t required);
int16_t Creature_Effect(
    ITEM_INFO *item, BITE_INFO *bite,
    int16_t (*spawn)(
        int32_t x, int32_t y, int32_t z, int16_t speed, int16_t yrot,
        int16_t room_num));
bool Creature_CheckBaddieOverlap(int16_t item_num);
bool Creature_Animate(int16_t item_num, int16_t angle, int16_t tilt);
