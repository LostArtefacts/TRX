#pragma once

#include "../config/types.h"
#include "./music/ids.h"

#include <stdint.h>

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
bool Gym_CanPlayMusicTrack(MUSIC_ID *track_id);
