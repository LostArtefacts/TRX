#pragma once

#include "../game_flow/types.h"
#include "./types.h"

// Loading a saved game is divided into two phases. First, the game reads the
// savegame file contents to look for the level number. The rest of the save
// data is stored in a special buffer in the g_GameInfo. Then the engine
// continues to execute the normal game flow and loads the specified level.
// Second phase occurs after everything finishes loading, e.g. items,
// creatures, triggers etc., and is what actually sets Lara's health, creatures
// status, triggers, inventory etc.

void Savegame_RegisterStrategy(SAVEGAME_STRATEGY strategy);
void Savegame_Init(void);
void Savegame_Shutdown(void);
bool Savegame_IsInitialised(void);
void Savegame_ScanSavedGames(void);

SAVEGAME_VERSION Savegame_GetInitialVersion(void);
void Savegame_SetInitialVersion(SAVEGAME_VERSION version);
int32_t Savegame_GetCounter(void);
int32_t Savegame_GetTotalCount(void);
int32_t Savegame_GetLevelNumber(int32_t slot_num);
bool Savegame_IsSlotFree(int32_t slot_num);
bool Savegame_RestartAvailable(int32_t slot_num);

// Remembers the slot used when the player starts a loaded game.
// Persists across level reloads.
void Savegame_BindSlot(int32_t slot_num);

// Removes the binding of the current slot. Used when the player exits to
// title, issues a command like `/play` etc.
void Savegame_UnbindSlot(void);

// Returns the currently bound slot number. If there is none, returns -1.
int32_t Savegame_GetBoundSlot(void);

// Returns the most recently created save slot number. If there is none,
// returns -1.
int32_t Savegame_GetMostRecentlyCreatedSlot(void);

// Returns the most recently used slot save number. If there is none, returns
// -1.
int32_t Savegame_GetMostRecentlyUsedSlot(void);

void Savegame_ProcessItemsBeforeLoad(void);
void Savegame_ProcessItemsBeforeSave(void);
bool Savegame_Load(int32_t slot_num);
bool Savegame_Save(int32_t slot_num);
bool Savegame_UpdateDeathCounters(int32_t slot_num, int32_t death_count);
bool Savegame_LoadOnlyResumeInfo(int32_t slot_num);

void Savegame_InitCurrentInfo(void);
void Savegame_SetCurrentInfo(int32_t current_slot, int32_t src_slot);
RESUME_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *level);
const SAVEGAME_INFO *Savegame_GetSavegameInfo(int32_t slot_num);
void Savegame_ResetCurrentInfo(const GF_LEVEL *level);
void Savegame_CarryCurrentInfoToNextLevel(
    const GF_LEVEL *src_level, const GF_LEVEL *dst_level);
void Savegame_PersistGameToCurrentInfo(const GF_LEVEL *level);
void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *level);

void Savegame_SetDefaultStats(const GF_LEVEL *level, STATS_COMMON stats);
STATS_COMMON Savegame_GetDefaultStats(const GF_LEVEL *level);

extern int32_t Savegame_GetSlotCount(void);
extern void Savegame_HighlightNewestSlot(void);

#define REGISTER_SAVEGAME_STRATEGY(strategy_)                                  \
    __attribute__((__constructor__)) static void M_Register(void)              \
    {                                                                          \
        Savegame_RegisterStrategy(strategy_);                                  \
    }
