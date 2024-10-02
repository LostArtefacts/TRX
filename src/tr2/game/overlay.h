#pragma once

#include "global/types.h"

bool __cdecl Overlay_FlashCounter(void);
void __cdecl Overlay_DrawAssaultTimer(void);
void __cdecl Overlay_DrawGameInfo(bool pickup_state);
void __cdecl Overlay_DrawHealthBar(bool flash_state);
void __cdecl Overlay_DrawAirBar(bool flash_state);
void Overlay_HideGameInfo(void);
void __cdecl Overlay_MakeAmmoString(char *string);
void __cdecl Overlay_DrawAmmoInfo(void);
void __cdecl Overlay_InitialisePickUpDisplay(void);
void __cdecl Overlay_DrawPickups(bool pickup_state);
void __cdecl Overlay_AddDisplayPickup(int16_t object_id);
void __cdecl Overlay_DisplayModeInfo(const char *string);
void __cdecl Overlay_DrawModeInfo(void);
