#pragma once

#include <libtrx/game/stats.h>

void Stats_ObserveRoomsLoad(void);
void Stats_ComputeFinal(GF_LEVEL_TYPE level_type, FINAL_STATS *stats);

void Stats_AddPickup(void);
