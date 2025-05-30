#pragma once

#include <stdint.h>

bool Lara_Cheat_GiveAllKeys(void);
bool Lara_Cheat_GiveAllGuns(void);
bool Lara_Cheat_GiveAllItems(void);
extern bool Lara_Cheat_EnterFlyMode(void);
extern bool Lara_Cheat_ExitFlyMode(void);
extern bool Lara_Cheat_KillEnemy(int16_t item_num);
void Lara_Cheat_EndLevel(void);
extern bool Lara_Cheat_Teleport(
    int32_t x, int32_t y, int32_t z, int16_t room_num);
