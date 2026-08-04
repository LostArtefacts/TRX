#pragma once

#include <trx/game/game_flow/types.h>
#include <trx/game/savegame/types.h>

// What each level of the playthrough keeps for Lara's return - her loadout,
// her health, the stats it gathered - held one entry per level of the main
// table. It belongs to the playthrough rather than to the save files: a save
// writes it out and a load reads it back, but nothing else may take it down
// while a game is running.

void Savegame_Resume_Init(void);
void Savegame_Resume_Shutdown(void);

void Savegame_InitCurrentInfo(void);
void Savegame_SetCurrentInfo(int32_t current_slot, int32_t src_slot);
RESUME_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *level);
void Savegame_ResetCurrentInfo(const GF_LEVEL *level);
void Savegame_CarryCurrentInfoToNextLevel(
    const GF_LEVEL *src_level, const GF_LEVEL *dst_level);
void Savegame_PersistGameToCurrentInfo(const GF_LEVEL *level);
void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *level);
int32_t Savegame_GetCompletedLevelCount(void);
