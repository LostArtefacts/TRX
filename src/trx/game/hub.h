#pragma once

#include <trx/game/game_flow/types.h>

void Hub_SetNextLevelIndex(int32_t level_idx);
const GF_LEVEL *Hub_GetNextLevel(void);
// Returns the next level index without consuming it, or 0 for none.
int32_t Hub_GetNextLevelIndex(void);
void Hub_SetLaraStartIndex(int32_t start_idx);
int32_t Hub_GetLaraStartIndex(void);
void Hub_InitialiseLaraStart(void);
