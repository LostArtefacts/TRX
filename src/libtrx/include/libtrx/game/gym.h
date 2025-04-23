#pragma once

#include "../config/types.h"

#include <stdint.h>

extern bool Gym_IsAccessible(void);
void Gym_SetInventoryOpenEnabled(bool enabled);
bool Gym_IsInventoryOpenEnabled(void);

bool Gym_HasAssaultStats(void);
bool Gym_IsAssaultTimerDisplay(void);
bool Gym_IsAssaultTimerActive(void);
ASSAULT_STATS Gym_GetAssaultStats(void);
void Gym_SetAssaultStats(ASSAULT_STATS stats);

void Gym_ResetAssault(void);
void Gym_StartAssault(void);
void Gym_StopAssault(void);
void Gym_FinishAssault(void);
