#pragma once

#include "global/types.h"

void __cdecl Inv_InitColors(void);
void __cdecl Inv_Construct(void);
int32_t __cdecl Inv_Display(int32_t inventory_mode);
void __cdecl Inv_SelectMeshes(INVENTORY_ITEM *inv_item);
int32_t __cdecl Inv_AnimateInventoryItem(INVENTORY_ITEM *inv_item);
void __cdecl Inv_DrawInventoryItem(INVENTORY_ITEM *inv_item);
GAME_OBJECT_ID Inv_GetItemOption(GAME_OBJECT_ID object_id);
void __cdecl Inv_DoInventoryPicture(void);
void __cdecl Inv_DoInventoryBackground(void);
void __cdecl Inv_RingIsOpen(RING_INFO *ring);
void __cdecl Inv_RingIsNotOpen(RING_INFO *ring);
void __cdecl Inv_RingNotActive(const INVENTORY_ITEM *inv_item);
void __cdecl Inv_RingActive(void);
void __cdecl Inv_RemoveInventoryText(void);
