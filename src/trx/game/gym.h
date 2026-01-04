#pragma once

#include <trx/config/types.h>
#include <trx/game/music/ids.h>

#include <stdint.h>

void Gym_SetInventoryOpenEnabled(bool enabled);
bool Gym_IsInventoryOpenEnabled(void);

bool Gym_HasAssaultStats(void);
bool Gym_IsAssaultTimerDisplay(void);
bool Gym_IsAssaultTimerActive(void);
ASSAULT_STATS Gym_GetAssaultStats(void);

// TR3 assault course extensions (targets + penalties).
void Gym_Control(void);
void Gym_Assault_AddPenaltySeconds(int32_t seconds);
void Gym_Assault_DecreaseTargetCount(void);
int32_t Gym_Assault_GetPenaltyDisplayTimer(void);
int32_t Gym_Assault_GetPenaltyFrames(void);
int32_t Gym_Assault_GetTargetPenaltyFrames(void);
bool Gym_Assault_OnPadContact(bool on_ground);

void Gym_ResetAssault(void);
void Gym_StartAssault(void);
void Gym_StopAssault(void);
void Gym_FinishAssault(void);

// Potentially converts the requested track id based on Lara's state. Returns
// true if the track should be played.
bool Gym_CanPlayMusicTrack(MUSIC_ID *track_id);
