#ifndef T1M_GAME_LIGHTNING_H
#define T1M_GAME_LIGHTNING_H

#include "game/types.h"
#include <stdint.h>

void DrawLightning(ITEM_INFO* item);
void InitialiseLightning(int16_t item_num);
void LightningControl(int16_t item_num);
void LightningCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);

void InitialiseThorsHandle(int16_t item_num);
void ThorsHandleControl(int16_t item_num);
void ThorsHandleCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void ThorsHeadCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);

void T1MInjectGameLightning();

#endif
