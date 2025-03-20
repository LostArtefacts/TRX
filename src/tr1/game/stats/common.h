#pragma once

#include "game/stats/types.h"
#include "global/types.h"

void Stats_ObserveRoomsLoad(void);
void Stats_CalculateStats(void);
int32_t Stats_GetPickups(void);
int32_t Stats_GetKillables(void);
int32_t Stats_GetSecrets(void);
void Stats_ComputeFinal(GF_LEVEL_TYPE level_type, FINAL_STATS *stats);
bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type);

void Stats_StartTimer(void);
void Stats_UpdateTimer(void);
void Stats_UpdateSecrets(LEVEL_STATS *stats);
