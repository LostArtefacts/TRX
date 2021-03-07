#ifndef T1M_GAME_PICKUP_H
#define T1M_GAME_PICKUP_H

#include <stdint.h>

void AnimateLaraUntil(ITEM_INFO *lara_item, int32_t goal);
void PickUpCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void PickUpScionCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void PickUpScion4Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void MidasCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void SwitchCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void SwitchCollision2(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void KeyHoleCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void PuzzleHoleCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void SwitchControl(int16_t item_num);
int32_t SwitchTrigger(int16_t item_num, int16_t timer);
int32_t KeyTrigger(int16_t item_num);
int32_t PickupTrigger(int16_t item_num);
void PickUpSaveGameCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void InitialiseSaveGameItem(int16_t item_num);
void ControlSaveGameItem(int16_t item_num);

void T1MInjectGamePickup();

#endif
