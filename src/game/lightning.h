#ifndef T1M_GAME_LIGHTNING_H
#define T1M_GAME_LIGHTNING_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define ThorsHandleControl      ((void          (*)(int16_t item_num))0x00429F30)
#define ThorsHandleCollision    ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0042A1F0)
#define ThorsHeadCollision      ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0042A240)
// clang-format on

void DrawLightning(ITEM_INFO* item);
void InitialiseLightning(int16_t item_num);
void LightningControl(int16_t item_num);
void LightningCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);

void InitialiseThorsHandle(int16_t item_num);

void T1MInjectGameLightning();

#endif
