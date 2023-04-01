#pragma once

#include "global/types.h"

#include <stdint.h>

struct BAR_INFO;

void Overlay_Init(void);

void Overlay_BarSetHealthTimer(int16_t health_bar_timer);
void Overlay_BarHealthTimerTick(void);
void Overlay_BarDraw(struct BAR_INFO *bar_info);
BAR_LOCATION Overlay_BarGetHealthLocation();
void Overlay_BarDrawHealth(void);
void Overlay_BarDrawAir(void);
void Overlay_BarDrawEnemy(void);
void Overlay_RemoveAmmoText(void);
void Overlay_DrawAmmoInfo(void);
void Overlay_DrawPickups(void);
void Overlay_DrawFPSInfo();
void Overlay_DrawGameInfo(void);

void Overlay_AddPickup(int16_t object_num);

void Overlay_MakeAmmoString(char *string);
void Overlay_SetFPSBarAware(bool medi_aware);
