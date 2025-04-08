#pragma once

#include "../collision.h"
#include "./types.h"

void Creature_Initialise(int16_t item_num);
bool Creature_Activate(int16_t item_num);
void Creature_AIInfo(ITEM *item, AI_INFO *info);
bool Creature_EnsureHabitat(
    int16_t item_num, int32_t *wh, const HYBRID_INFO *info);
void Creature_Mood(const ITEM *item, const AI_INFO *info, bool violent);

extern bool Creature_IsHostile(const ITEM *item);
extern void Creature_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
extern int16_t Creature_Turn(ITEM *item, int16_t maximum_turn);
extern void Creature_Tilt(ITEM *item, int16_t angle);
extern void Creature_Head(ITEM *item, int16_t required);
extern bool Creature_Animate(int16_t item_num, int16_t angle, int16_t tilt);
extern int16_t Creature_Effect(
    const ITEM *item, const BITE *bite,
    int16_t (*spawn)(
        int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
        int16_t room_num));
