#pragma once

#include "global/types.h"

#include <libtrx/game/game_flow.h>
#include <libtrx/game/stats.h>

void Stats_UpdateTimer(void);
void Stats_CalculateStats(void);
void Stats_MarkSecretCollected(const ITEM *item);
FINAL_STATS Stats_ComputeFinalStats(GF_LEVEL_TYPE level_type);

int32_t Stats_GetMaxSecrets(void);
bool Stats_CheckAllLevelSecretsCollected(void);
bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type);
uint32_t Stats_ReserveSecretBit(OBJECT_ID object_id);
OBJECT_ID Stats_GetSecretObject(int32_t secret_idx);
