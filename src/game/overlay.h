#pragma once

#include <stdint.h>

void Overlay_Init();

void Overlay_BarSetHealthTimer(int16_t health_bar_timer);
void Overlay_BarHealthTimerTick();
void Overlay_BarDrawHealth();
void Overlay_BarDrawAir();
void Overlay_BarDrawEnemy();
void Overlay_DrawAmmoInfo();
void Overlay_DrawPickups();
void Overlay_DrawFPSInfo();
void Overlay_DrawGameInfo();

void Overlay_AddPickup(int16_t object_num);

void Overlay_MakeAmmoString(char *string);
