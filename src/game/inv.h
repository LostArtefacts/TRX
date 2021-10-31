#ifndef T1M_GAME_INV_H
#define T1M_GAME_INV_H

#include "global/types.h"

#include <stdint.h>

extern int16_t InvKeysCurrent;
extern int16_t InvKeysObjects;
extern int16_t InvKeysQtys[24];
extern INVENTORY_ITEM *InvKeysList[23];

extern int16_t InvMainCurrent;
extern int16_t InvMainObjects;
extern int16_t InvMainQtys[24];
extern INVENTORY_ITEM *InvMainList[23];

extern int16_t InvOptionCurrent;
extern int16_t InvOptionObjects;
extern INVENTORY_ITEM *InvOptionList[5];

extern INVENTORY_ITEM InvItemCompass;
extern INVENTORY_ITEM InvItemMedi;
extern INVENTORY_ITEM InvItemBigMedi;
extern INVENTORY_ITEM InvItemLeadBar;
extern INVENTORY_ITEM InvItemPickup1;

extern TEXTSTRING *InvItemText[2];
extern TEXTSTRING *InvRingText;

int32_t Display_Inventory(int inv_mode);
void Construct_Inventory();
void SelectMeshes(INVENTORY_ITEM *inv_item);
int32_t AnimateInventoryItem(INVENTORY_ITEM *inv_item);
void DrawInventoryItem(INVENTORY_ITEM *inv_item);

void InitColours();
void RingIsOpen(RING_INFO *ring);
void RingIsNotOpen(RING_INFO *ring);
void RingNotActive(INVENTORY_ITEM *inv_item);
void RingActive();

int32_t Inv_AddItem(int32_t item_num);
void Inv_InsertItem(INVENTORY_ITEM *inv_item);
int32_t Inv_RequestItem(int item_num);
void Inv_RemoveAllItems();
int32_t Inv_RemoveItem(int32_t item_num);
int32_t Inv_GetItemOption(int32_t item_num);
void RemoveInventoryText();
void Inv_RingInit(
    RING_INFO *ring, int16_t type, INVENTORY_ITEM **list, int16_t qty,
    int16_t current, IMOTION_INFO *imo);
void Inv_RingGetView(RING_INFO *a1, PHD_3DPOS *viewer);
void Inv_RingLight(RING_INFO *ring);
void Inv_RingCalcAdders(RING_INFO *ring, int16_t rotation_duration);
void Inv_RingDoMotions(RING_INFO *ring);
void Inv_RingRotateLeft(RING_INFO *ring);
void Inv_RingRotateRight(RING_INFO *ring);
void Inv_RingMotionInit(
    RING_INFO *ring, int16_t frames, int16_t status, int16_t status_target);
void Inv_RingMotionSetup(
    RING_INFO *ring, int16_t status, int16_t status_target, int16_t frames);
void Inv_RingMotionRadius(RING_INFO *ring, int16_t target);
void Inv_RingMotionRotation(RING_INFO *ring, int16_t rotation, int16_t target);
void Inv_RingMotionCameraPos(RING_INFO *ring, int16_t target);
void Inv_RingMotionCameraPitch(RING_INFO *ring, int16_t target);
void Inv_RingMotionItemSelect(RING_INFO *ring, INVENTORY_ITEM *inv_item);
void Inv_RingMotionItemDeselect(RING_INFO *ring, INVENTORY_ITEM *inv_item);

#endif
