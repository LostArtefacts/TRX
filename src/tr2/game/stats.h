#pragma once

#include "global/types.h"

#include <libtrx/game/game_flow.h>

void Stats_StartTimer(void);
void Stats_UpdateTimer(void);
void Stats_Reset(void);
void Stats_CalculateStats(void);
int32_t Stats_GetSecrets(void);
FINAL_STATS Stats_ComputeFinalStats(GF_LEVEL_TYPE level_type);
bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type);
