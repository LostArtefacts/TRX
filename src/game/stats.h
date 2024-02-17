#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct TOTAL_STATS {
    uint32_t timer;
    uint32_t death_count;
    uint32_t player_kill_count;
    uint16_t player_secret_count;
    uint16_t player_pickup_count;
    uint32_t total_kill_count;
    uint16_t total_secret_count;
    uint16_t total_pickup_count;
} TOTAL_STATS;

void Stats_ObserveRoomsLoad(void);
void Stats_ObserveItemsLoad(void);
void Stats_CalculateStats(void);
int32_t Stats_GetPickups(void);
int32_t Stats_GetKillables(void);
int32_t Stats_GetSecrets(void);
void Stats_ComputeTotal(GAMEFLOW_LEVEL_TYPE level_type, TOTAL_STATS *stats);
bool Stats_CheckAllSecretsCollected(GAMEFLOW_LEVEL_TYPE level_type);
