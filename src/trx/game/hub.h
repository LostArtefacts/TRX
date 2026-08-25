#pragma once

#include <trx/game/game_flow/types.h>

void Hub_SetNextLevelIndex(int32_t level_idx);
const GF_LEVEL *Hub_GetNextLevel(void);
void Hub_SetLaraStartIndex(int32_t start_idx);
void Hub_InitialiseLaraStart(void);
