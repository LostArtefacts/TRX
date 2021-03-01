#ifndef T1M_GAME_INV_H
#define T1M_GAME_INV_H

#include "types.h"
#include <stdint.h>

// clang-format off
#define RingIsOpen                  ((void          (*)(RING_INFO* ring))0x00420000)
#define RingIsNotOpen               ((void          (*)(RING_INFO* ring))0x00420150)
#define RingNotActive               ((void          (*)(INVENTORY_ITEM* inv_item))0x004201D0)
#define RingActive                  ((void          (*)())0x00420980)
#define RemoveInventoryText         ((void          (*)())0x00421550)
#define Inv_AddItem                 ((int32_t       (*)(int16_t item_num))0x004209C0)
#define Inv_RemoveAllItems          ((void          (*)())0x00421280)
#define Inv_RemoveItem              ((void          (*)(int16_t item_num))0x004212A0)
#define Inv_RequestItem             ((int32_t       (*)(int16_t item_num))0x00421200)
#define Inv_RingInit                ((void          (*)(RING_INFO* ring, int16_t type, INVENTORY_ITEM** list, int16_t qty, int16_t current, IMOTION_INFO* imo))0x00421580)
#define Inv_RingGetView             ((void          (*)(RING_INFO* ring, PHD_3DPOS* viewer))0x00421700)
#define Inv_RingLight               ((void          (*)(RING_INFO* ring))0x00421760)
#define Inv_RingCalcAdders          ((void          (*)(RING_INFO* ring, int16_t rotation_duration))0x004217A0)
#define Inv_RingDoMotions           ((void          (*)(RING_INFO* ring))0x004217D0)
#define Inv_RingRotateLeft          ((void          (*)(RING_INFO* ring))0x00421910)
#define Inv_RingRotateRight         ((void          (*)(RING_INFO* ring))0x00421940)
#define Inv_RingMotionSetup         ((void          (*)(RING_INFO* ring, int16_t status, int16_t status_target, int16_t frames))0x00421970)
#define Inv_RingMotionRadius        ((void          (*)(RING_INFO* ring, int16_t target))0x004219A0)
#define Inv_RingMotionRotation      ((void          (*)(RING_INFO* ring, int16_t rotation, int16_t target))0x004219D0)
#define Inv_RingMotionCameraPos     ((void          (*)(RING_INFO* ring, int16_t target))0x00421A00)
#define Inv_RingMotionCameraPitch   ((void          (*)(RING_INFO* ring, int16_t target))0x00421A30)
#define Inv_RingMotionItemSelect    ((void          (*)(RING_INFO* ring, INVENTORY_ITEM* inv_item))0x00421A50)
#define Inv_RingMotionItemDeselect  ((void          (*)(RING_INFO* ring, INVENTORY_ITEM* inv_item))0x00421AB0)
// clang-format on

int32_t Display_Inventory(int inv_mode);
void Construct_Inventory();
void SelectMeshes(INVENTORY_ITEM* inv_item);
int32_t AnimateInventoryItem(INVENTORY_ITEM* inv_item);
void DrawInventoryItem(INVENTORY_ITEM* inv_item);
int32_t GetDebouncedInput(int32_t input);
void InitColours();

void T1MInjectGameInvEntry();
void T1MInjectGameInvFunc();

#endif
