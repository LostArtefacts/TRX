#pragma once

#include "global/types.h"

void __cdecl Option_DoInventory(INVENTORY_ITEM *item);
void __cdecl Option_ShutdownInventory(INVENTORY_ITEM *item);

void __cdecl Option_Passport(INVENTORY_ITEM *item);
void __cdecl Option_Passport_Shutdown(void);

void __cdecl Option_Detail(INVENTORY_ITEM *item);
void __cdecl Option_Detail_Shutdown(void);

void __cdecl Option_Sound(INVENTORY_ITEM *item);
void __cdecl Option_Sound_Shutdown(void);

void __cdecl Option_Controls_FlashConflicts(void);
void __cdecl Option_Controls_DefaultConflict(void);
void __cdecl Option_Controls(INVENTORY_ITEM *item);
void __cdecl Option_Controls_Shutdown(void);
void __cdecl Option_Controls_ShowControls(void);
void __cdecl Option_Controls_UpdateText(void);

void __cdecl Option_Compass(INVENTORY_ITEM *item);
void __cdecl Option_Compass_Shutdown(void);
