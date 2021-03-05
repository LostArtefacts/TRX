#ifndef T1M_GAME_PICKUP_H
#define T1M_GAME_PICKUP_H

#include <stdint.h>

// clang-format off
#define PickUpScionCollision    ((void      (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433240)
#define PickUpScion4Collision   ((void      (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x004333B0)
#define MidasCollision          ((void      (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x004334C0)
#define SwitchCollision         ((void      (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x004336F0)
#define SwitchCollision2        ((void      (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433810)
#define KeyHoleCollision        ((void      (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433900)
#define PuzzleHoleCollision     ((void      (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x00433B40)
#define SwitchControl           ((void      (*)(int16_t item_num))0x00433DE0)
#define SwitchTrigger           ((int32_t   (*)(int16_t item_num, int16_t timer))0x00433E20)
#define PickupTrigger           ((int32_t   (*)(int16_t item_num))0x00433EF0)
#define InitialiseSaveGameItem  ((void      (*)(int16_t item_num))0x00433F30)
// clang-format on

void AnimateLaraUntil(ITEM_INFO* lara_item, int32_t goal);
void PickUpCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
int32_t KeyTrigger(int16_t item_num);

void T1MInjectGamePickup();

#endif
