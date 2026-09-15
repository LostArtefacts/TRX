#pragma once

#include <trx/game/game_flow/types.h>

void GF_InventoryModifier_Scan(const GF_LEVEL *level);
void GF_InventoryModifier_Apply(const GF_LEVEL *level, GF_INV_TYPE type);
void GF_InventoryModifier_ApplyToResumeInfo(const GF_LEVEL *level);

// Takes the plot items away from what the next level keeps, where the level
// being left declares a hub reset into it.
void GF_InventoryModifier_ApplyHubReset(
    const GF_LEVEL *level, const GF_LEVEL *next_level);

int32_t GF_GetSecretRewardCount(const GF_LEVEL *level);
