#pragma once

#include <stdint.h>

#define MAX_ASSAULT_TIMES 10

typedef struct {
    uint32_t best_time[MAX_ASSAULT_TIMES];
    uint32_t best_finish[MAX_ASSAULT_TIMES];
    uint32_t finish_count;
} ASSAULT_STATS;

bool Gym_IsAccessible(void);
void Gym_SetInventoryOpenEnabled(bool enabled);
bool Gym_IsInventoryOpenEnabled(void);

bool Gym_IsAssaultTimerDisplay(void);
bool Gym_IsAssaultTimerActive(void);
ASSAULT_STATS Gym_GetAssaultStats(void);

void Gym_ResetAssault(void);
void Gym_StartAssault(void);
void Gym_StopAssault(void);
void Gym_FinishAssault(void);
