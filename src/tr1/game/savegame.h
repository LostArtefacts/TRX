#pragma once

#include "global/types.h"

#include <libtrx/game/savegame.h>

#include <stdint.h>

// Loading a saved game is divided into two phases. First, the game reads the
// savegame file contents to look for the level number. The rest of the save
// data is stored in a special buffer in the g_GameInfo. Then the engine
// continues to execute the normal game flow and loads the specified level.
// Second phase occurs after everything finishes loading, e.g. items,
// creatures, triggers etc., and is what actually sets Lara's health, creatures
// status, triggers, inventory etc.

#define SAVEGAME_CURRENT_VERSION 7

typedef enum {
    VERSION_LEGACY = -1,
    VERSION_0 = 0,
    VERSION_1 = 1,
    VERSION_2 = 2,
    VERSION_3 = 3,
    VERSION_4 = 4,
    VERSION_5 = 5,
    VERSION_6 = 6,

    VERSION_7 = 7,
    // Added extra footer after the compressed BSON structure for quicker
    // access to essential data, such as the level counter and the level title,
    // without the need to parse the entire BSON document.
    // Added TR2+ stats ammo hits/used, health packs used, distance travelled.
} SAVEGAME_VERSION;

void Savegame_Init(void);
void Savegame_Shutdown(void);
bool Savegame_IsInitialised(void);

void Savegame_InitCurrentInfo(void);
void Savegame_SetCurrentInfo(int32_t current_slot, int32_t src_slot);

int32_t Savegame_GetLevelNumber(int32_t slot_num);

bool Savegame_UpdateDeathCounters(int32_t slot_num, int32_t death_count);
bool Savegame_LoadOnlyResumeInfo(int32_t slot_num);

void Savegame_ScanSavedGames(void);
void Savegame_FillAvailableSaves(REQUEST_INFO *req);
void Savegame_FillAvailableLevels(REQUEST_INFO *req);
void Savegame_HighlightNewestSlot(void);
bool Savegame_RestartAvailable(int32_t slot_num);

RESUME_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *level);

void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *level);
void Savegame_ResetCurrentInfo(const GF_LEVEL *level);
void Savegame_CarryCurrentInfoToNextLevel(
    const GF_LEVEL *src_level, const GF_LEVEL *dst_level);

// Persist Lara's inventory to the current info.
// Used to carry over Lara's inventory between levels.
void Savegame_PersistGameToCurrentInfo(const GF_LEVEL *level);

void Savegame_ProcessItemsBeforeLoad(void);
void Savegame_ProcessItemsBeforeSave(void);

SAVEGAME_VERSION Savegame_GetInitialVersion(void);
void Savegame_SetInitialVersion(SAVEGAME_VERSION version);
int32_t Savegame_GetCounter(void);
int32_t Savegame_GetTotalCount(void);
