#pragma once

#include <stdbool.h>
#include <stdint.h>

extern int32_t Savegame_GetSlotCount(void);
extern bool Savegame_IsSlotFree(int32_t slot_num);
extern bool Savegame_Load(int32_t slot_num);
extern bool Savegame_Save(int32_t slot_num);
