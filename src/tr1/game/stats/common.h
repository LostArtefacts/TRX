#pragma once

#include "global/types.h"

#include <libtrx/game/stats.h>

void Stats_ObserveRoomsLoad(void);
void Stats_CalculateStats(void);
int32_t Stats_GetMaxPickups(void);
int32_t Stats_GetMaxKillables(void);
int32_t Stats_GetMaxSecrets(void);
void Stats_ComputeFinal(GF_LEVEL_TYPE level_type, FINAL_STATS *stats);
bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type);

void Stats_UpdateTimer(void);

void Stats_AddPickup(void);
