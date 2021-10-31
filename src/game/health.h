#ifndef T1M_GAME_HEALTH_H
#define T1M_GAME_HEALTH_H

#include <stdint.h>

void MakeAmmoString(char *string);
void DrawAmmoInfo();
void DrawFPSInfo();
void DrawGameInfo();
void DrawHealthBar();
void DrawEnemyBar();
void DrawAirBar();
void AddDisplayPickup(int16_t object_num);
void InitialisePickUpDisplay();
void DrawPickups();

#endif
