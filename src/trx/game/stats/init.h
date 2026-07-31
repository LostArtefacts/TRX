#pragma once

#include <trx/game/game_flow/types.h>
#include <trx/game/stats/types.h>

// Whether the scan has anything to say about this level. Only the levels of
// the game proper are scanned, so a title, a cutscene or a demo has no maxima
// and asking for them is a programming error.
bool Stats_HasLevelMaxStats(const GF_LEVEL *level);

LEVEL_MAX_STATS *Stats_GetLevelMaxStats(const GF_LEVEL *level);

bool Stats_GameHasCrystals(void);

void Stats_CalculateMaxStats(void);
