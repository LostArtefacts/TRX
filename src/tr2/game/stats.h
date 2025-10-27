#pragma once

#include <libtrx/game/items/types.h>
#include <libtrx/game/stats.h>

FINAL_STATS Stats_ComputeFinalStats(GF_LEVEL_TYPE level_type);

int32_t Stats_GetMaxSecrets(void);
bool Stats_CheckAllLevelSecretsCollected(void);
uint32_t Stats_ReserveSecretBit(OBJECT_ID object_id);
OBJECT_ID Stats_GetSecretObject(int32_t secret_idx);
