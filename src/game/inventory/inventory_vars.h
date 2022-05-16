#pragma once

#include "global/types.h"

#include <stdbool.h>
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
