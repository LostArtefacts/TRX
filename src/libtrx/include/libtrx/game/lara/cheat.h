#pragma once

#include <stdbool.h>
#include <stdint.h>

extern bool Lara_Cheat_GiveAllKeys(void);
extern bool Lara_Cheat_GiveAllGuns(void);
extern bool Lara_Cheat_GiveAllItems(void);
extern bool Lara_Cheat_EnterFlyMode(void);
extern bool Lara_Cheat_ExitFlyMode(void);
extern bool Lara_Cheat_KillEnemy(int16_t item_num);
extern void Lara_Cheat_EndLevel(void);
extern bool Lara_Cheat_Teleport(int32_t x, int32_t y, int32_t z);
