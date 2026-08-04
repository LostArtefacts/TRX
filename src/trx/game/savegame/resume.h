#pragma once

#include <trx/game/game_flow/types.h>
#include <trx/game/savegame/types.h>

// What each level of the playthrough keeps for Lara's return - her loadout,
// her health, the stats it gathered - held one entry per level of the main
// table. It belongs to the playthrough rather than to the save files: a save
// writes it out and a load reads it back, but nothing else may take it down
// while a game is running.

void SG_Resume_Init(void);
void SG_Resume_Shutdown(void);

void SG_Resume_ResetAllEntries(void);
int32_t SG_Resume_CountCompletedLevels(void);

RESUME_INFO *SG_Resume_GetEntry(const GF_LEVEL *level);
void SG_Resume_ResetEntry(const GF_LEVEL *level);
void SG_Resume_CarryEntry(const GF_LEVEL *src_level, const GF_LEVEL *dst_level);
void SG_Resume_ApplyRulesToEntry(const GF_LEVEL *level);

// Takes the running game into the level's entry.
void SG_Resume_StoreGameToEntry(const GF_LEVEL *level);

// Mirrors a level's entry into the one the GFL_CURRENT level holds, which is
// where a save looks for the game it is about to write.
void SG_Resume_MirrorCurrentEntry(const GF_LEVEL *level);
