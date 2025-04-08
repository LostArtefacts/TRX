#pragma once

#include "global/types.h"

#include <libtrx/game/creature.h>

int32_t Creature_CheckBaddieOverlap(int16_t item_num);
void Creature_Die(int16_t item_num, bool explode);
void Creature_Float(int16_t item_num);
void Creature_Underwater(ITEM *item, int32_t depth);
int32_t Creature_Vault(
    int16_t item_num, int16_t angle, int32_t vault, int32_t shift);
void Creature_Kill(
    ITEM *item, int32_t kill_anim, int32_t kill_state, int32_t lara_kill_state);
void Creature_GetBaddieTarget(int16_t item_num, bool goody);
int32_t Creature_CanTargetEnemy(const ITEM *item, const AI_INFO *info);
bool Creature_IsAlly(const ITEM *item);
int32_t Creature_ShootAtLara(
    ITEM *item, const AI_INFO *info, const BITE *gun, int16_t extra_rotation,
    int32_t damage);
