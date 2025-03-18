#pragma once

#include "global/types_decomp.h"

bool Gym_IsAccessible(void);
void Gym_SetInventoryOpenEnabled(bool enabled);
bool Gym_IsInventoryOpenEnabled(void);

ASSAULT_STATS Gym_GetAssaultStats(void);

void Gym_ResetAssault(void);
void Gym_StartAssault(void);
void Gym_StopAssault(void);
void Gym_FinishAssault(void);
