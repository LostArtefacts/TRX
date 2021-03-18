#ifndef T1M_GAME_HEALTH_H
#define T1M_GAME_HEALTH_H

#include "game/types.h"

#include <stdint.h>

extern TEXTSTRING *AmmoText;

void MakeAmmoString(char *string);
void DrawAmmoInfo();
void DrawGameInfo();
void DrawHealthBar();
void DrawEnemyBar();
void DrawAirBar();
void AddDisplayPickup(int16_t object_num);
void InitialisePickUpDisplay();
void DrawPickups();

void T1MInjectGameHealth();

#endif
