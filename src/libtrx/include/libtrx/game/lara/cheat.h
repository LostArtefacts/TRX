#pragma once

#include "../math/types.h"

#include <stdint.h>

bool Lara_Cheat_GiveAllKeys(void);
bool Lara_Cheat_GiveAllGuns(void);
bool Lara_Cheat_GiveAllItems(void);
void Lara_Cheat_GetStuff(void);
void Lara_Cheat_EndLevel(void);
bool Lara_Cheat_KillEnemy(int16_t item_num);
bool Lara_Cheat_OpenNearestDoor(void);
bool Lara_Cheat_EnterFlyMode(void);
bool Lara_Cheat_ExitFlyMode(void);
bool Lara_Cheat_Teleport(XYZ_32 pos, int16_t room_num);
