#ifndef T1M_GAME_INV_H
#define T1M_GAME_INV_H

#include "global/types.h"

#include <stdint.h>

extern int16_t g_InvKeysCurrent;
extern int16_t g_InvKeysObjects;
extern int16_t g_InvKeysQtys[];
extern INVENTORY_ITEM *g_InvKeysList[];

extern int16_t g_InvMainCurrent;
extern int16_t g_InvMainObjects;
extern int16_t g_InvMainQtys[];
extern INVENTORY_ITEM *g_InvMainList[];

extern int16_t g_InvOptionCurrent;
extern int16_t g_InvOptionObjects;
extern INVENTORY_ITEM *g_InvOptionList[];

extern INVENTORY_ITEM g_InvItemCompass;
extern INVENTORY_ITEM g_InvItemMedi;
extern INVENTORY_ITEM g_InvItemBigMedi;
extern INVENTORY_ITEM g_InvItemLeadBar;
extern INVENTORY_ITEM g_InvItemPickup1;
extern INVENTORY_ITEM g_InvItemPickup2;
extern INVENTORY_ITEM g_InvItemScion;
extern INVENTORY_ITEM g_InvItemPuzzle1;
extern INVENTORY_ITEM g_InvItemPuzzle2;
extern INVENTORY_ITEM g_InvItemPuzzle3;
extern INVENTORY_ITEM g_InvItemPuzzle4;
extern INVENTORY_ITEM g_InvItemKey1;
extern INVENTORY_ITEM g_InvItemKey2;
extern INVENTORY_ITEM g_InvItemKey3;
extern INVENTORY_ITEM g_InvItemKey4;
extern INVENTORY_ITEM g_InvItemPistols;
extern INVENTORY_ITEM g_InvItemShotgun;
extern INVENTORY_ITEM g_InvItemMagnum;
extern INVENTORY_ITEM g_InvItemUzi;
extern INVENTORY_ITEM g_InvItemGrenade;
extern INVENTORY_ITEM g_InvItemPistolAmmo;
extern INVENTORY_ITEM g_InvItemShotgunAmmo;
extern INVENTORY_ITEM g_InvItemMagnumAmmo;
extern INVENTORY_ITEM g_InvItemUziAmmo;
extern INVENTORY_ITEM g_InvItemGame;
extern INVENTORY_ITEM g_InvItemDetails;
extern INVENTORY_ITEM g_InvItemSound;
extern INVENTORY_ITEM g_InvItemControls;
extern INVENTORY_ITEM g_InvItemLarasHome;

extern TEXTSTRING *g_InvItemText[];
extern TEXTSTRING *g_InvRingText;

int32_t Display_Inventory(int inv_mode);
void Construct_Inventory();
void SelectMeshes(INVENTORY_ITEM *inv_item);
int32_t AnimateInventoryItem(INVENTORY_ITEM *inv_item);
void DrawInventoryItem(INVENTORY_ITEM *inv_item);

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
