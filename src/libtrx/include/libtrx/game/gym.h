#pragma once

#include "../config/types.h"

#include <stdint.h>

extern bool Gym_IsAccessible(void);
void Gym_SetInventoryOpenEnabled(bool enabled);
bool Gym_IsInventoryOpenEnabled(void);

bool Gym_HasAssaultStats(void);
bool Gym_IsAssaultTimerDisplay(void);
bool Gym_IsAssaultTimerActive(void);
ASSAULT_STATS Gym_GetAssaultStats(void);

void Gym_ResetAssault(void);
void Gym_StartAssault(void);
void Gym_StopAssault(void);
void Gym_FinishAssault(void);

// Potentially converts the requested track id based on Lara's state. Returns
// true if the track should be played.
bool Gym_CanPlayMusicTrack(int16_t *track_id);
