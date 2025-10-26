#pragma once

#include <libtrx/game/game_flow/types.h>
#include <libtrx/game/stats.h>

void Stats_ObserveRoomsLoad(void);
void Stats_ComputeFinal(GF_LEVEL_TYPE level_type, FINAL_STATS *stats);
bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type);

void Stats_AddPickup(void);
