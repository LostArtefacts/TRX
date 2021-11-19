#ifndef T1M_GAME_OVERLAY_H
#define T1M_GAME_OVERLAY_H

#include <stdint.h>

void Overlay_Init();

void Overlay_DrawHealthBar();
void Overlay_DrawAirBar();
void Overlay_DrawEnemyBar();
void Overlay_DrawAmmoInfo();
void Overlay_DrawPickups();
void Overlay_DrawFPSInfo();
void Overlay_DrawGameInfo();

void Overlay_AddPickup(int16_t object_num);

void Overlay_MakeAmmoString(char *string);

#endif
