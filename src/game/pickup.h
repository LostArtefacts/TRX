#ifndef T1M_GAME_PICKUP_H
#define T1M_GAME_PICKUP_H

#include <stdint.h>

// clang-format off
#define SwitchControl           ((void      (*)(int16_t item_num))0x00433DE0)
#define SwitchTrigger           ((int32_t   (*)(int16_t item_num, int16_t timer))0x00433E20)
#define PickupTrigger           ((int32_t   (*)(int16_t item_num))0x00433EF0)
#define InitialiseSaveGameItem  ((void      (*)(int16_t item_num))0x00433F30)
// clang-format on

void AnimateLaraUntil(ITEM_INFO* lara_item, int32_t goal);
void PickUpCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void PickUpScionCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void PickUpScion4Collision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void MidasCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void SwitchCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void SwitchCollision2(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void KeyHoleCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
void PuzzleHoleCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
int32_t KeyTrigger(int16_t item_num);

void T1MInjectGamePickup();

#endif
