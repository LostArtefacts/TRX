#ifndef T1M_GAME_HEALTH_H
#define T1M_GAME_HEALTH_H

#include <stdint.h>

void MakeAmmoString(char *string);
void DrawAmmoInfo();
void DrawGameInfo();
void DrawHealthBar();
void DrawAirBar();
void AddDisplayPickup(int16_t object_num);
void InitialisePickUpDisplay();
void DrawPickups();

#ifdef T1M_FEAT_GAMEPLAY
void DrawEnemyBar();
#endif

void T1MInjectGameHealth();

#endif
