#ifndef T1M_GAME_BOX_H
#define T1M_GAME_BOX_H

#include "game/types.h"

#include <stdint.h>

void InitialiseCreature(int16_t item_num);
void CreatureAIInfo(ITEM_INFO *item, AI_INFO *info);
int32_t SearchLOT(LOT_INFO *LOT, int32_t expansion);
int32_t UpdateLOT(LOT_INFO *LOT, int32_t expansion);
void TargetBox(LOT_INFO *LOT, int16_t box_number);
int32_t StalkBox(ITEM_INFO *item, int16_t box_number);
int32_t EscapeBox(ITEM_INFO *item, int16_t box_number);
int32_t ValidBox(ITEM_INFO *item, int16_t zone_number, int16_t box_number);
void CreatureMood(ITEM_INFO *item, AI_INFO *info, int32_t violent);
int32_t CalculateTarget(PHD_VECTOR *target, ITEM_INFO *item, LOT_INFO *LOT);
int32_t CreatureCreature(int16_t item_num);
int32_t BadFloor(
    int32_t x, int32_t y, int32_t z, int16_t box_height, int16_t next_height,
    int16_t room_number, LOT_INFO *LOT);
int32_t CreatureAnimation(int16_t item_num, int16_t angle, int16_t tilt);
int16_t CreatureTurn(ITEM_INFO *item, int16_t maximum_turn);
void CreatureTilt(ITEM_INFO *item, int16_t angle);
void CreatureHead(ITEM_INFO *item, int16_t required);
int16_t CreatureEffect(
    ITEM_INFO *item, BITE_INFO *bite,
    int16_t (*generate)(
        int32_t x, int32_t y, int32_t z, int16_t speed, int16_t yrot,
        int16_t room_num));

void T1MInjectGameBox();

#endif
