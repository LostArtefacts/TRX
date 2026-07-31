#pragma once

#include <trx/game/inventory_ring/enum.h>
#include <trx/game/objects/ids.h>

#include <stdint.h>

// How many kinds of thing Lara can carry at once. The rings can draw fewer
// than this, so a level runs out of somewhere to show them first.
#define INV_MAX_ENTRIES 100

typedef struct {
    // The inventory option, which is what the rings draw. Several pickups can
    // share one, so this is what Inv_GetItemOption hands back rather than the
    // object lying in the level.
    OBJECT_ID object_id;
    int32_t qty;
} INVENTORY_ENTRY;

// What Lara is carrying. Holding no pointers, it is copied by assignment,
// which is how a level hands what she has to the next one.
typedef struct {
    INVENTORY_ENTRY entries[INV_MAX_ENTRIES];
    int32_t count;
} INVENTORY_STATE;

extern INVENTORY_MODE g_Inv_Mode;

// What Lara is carrying, which the rings are drawn from. Writing it puts her
// in front of a whole inventory at once, as arriving in a level does.
INVENTORY_STATE *Inv_GetState(void);
void Inv_SetState(const INVENTORY_STATE *state);

OBJECT_ID Inv_GetItemOption(OBJECT_ID obj_id);
OBJECT_ID Inv_GetItemPickup(OBJECT_ID obj_id);

int32_t Inv_RequestItem(OBJECT_ID obj_id);
void Inv_SetItemCount(OBJECT_ID obj_id, int32_t qty);
bool Inv_AddItem(OBJECT_ID obj_id);
bool Inv_AddItemNTimes(OBJECT_ID obj_id, int32_t qty);
bool Inv_RemoveItem(OBJECT_ID obj_id);
bool Inv_CanAddItem(OBJECT_ID obj_id);
void Inv_RemoveAllItems(void);
