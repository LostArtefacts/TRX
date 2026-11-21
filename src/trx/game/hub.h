#pragma once

#include <trx/core/math/types.h>
#include <trx/game/game_flow/types.h>

typedef struct {
    XYZ_32 pos;
    int16_t rot_y;
    int16_t room_num;
} HUB_START_POINT;

void Hub_SetNextLevelIndex(int32_t level_idx);
const GF_LEVEL *Hub_GetNextLevel(void);
// Returns the next level index without consuming it, or 0 for none.
int32_t Hub_GetNextLevelIndex(void);
void Hub_SetLaraStartIndex(int32_t start_idx);
int32_t Hub_GetLaraStartIndex(void);
void Hub_InitialiseLaraStart(const GF_LEVEL *level);

// Sets the places that a finish trigger can start Lara at in the level being
// loaded. A trigger's timer counts them from 1, in the order that the level
// file lists its AI objects in TR4, or its Lara start positions in TR1-3. The
// points must last as long as the level.
void Hub_SetStartPoints(const HUB_START_POINT *points, int32_t count);
