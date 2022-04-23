#pragma once

#include "global/types.h"

#include <stdint.h>

// Loading a saved game is divided into two phases. First, the game reads the
// savegame file contents to look for the level number. The rest of the save
// data is stored in a special buffer in the g_GameInfo. Then the engine
// continues to execute the normal game flow and loads the specified level.
// Second phase occurs after everything finishes loading, e.g. items,
// creatures, triggers etc., and is what actually sets Lara's health, creatures
// status, triggers, inventory etc.

#define SAVEGAME_VERSION_0 0
#define SAVEGAME_VERSION_1 1
#define SAVEGAME_CURRENT_VERSION 1

typedef enum SAVEGAME_FORMAT {
    SAVEGAME_FORMAT_LEGACY = 1,
    SAVEGAME_FORMAT_BSON = 2,
} SAVEGAME_FORMAT;

typedef struct SAVEGAME_INFO {
    SAVEGAME_FORMAT format;
    char *full_path;
    int32_t counter;
    int32_t level_num;
    char *level_title;
    uint16_t initial_version;
    bool restart;
    bool select_level;
} SAVEGAME_INFO;

void Savegame_InitCurrentInfo(void);

int32_t Savegame_GetLevelNumber(int32_t slot_num);

bool Savegame_Load(int32_t slot_num, GAME_INFO *game_info);
bool Savegame_Save(int32_t slot_num, GAME_INFO *game_info);
bool Savegame_UpdateDeathCounters(int32_t slot_num, GAME_INFO *game_info);
bool Savegame_LoadOnlyResumeInfo(int32_t slot_num, GAME_INFO *game_info);

void Savegame_ScanSavedGames(void);
void Savegame_ScanAvailableLevels(REQUEST_INFO *req);
bool Savegame_RestartAvailable();
void Savegame_Shutdown(void);

void Savegame_ApplyLogicToCurrentInfo(int level_num);
void Savegame_ResetCurrentInfo(int level_num);
void Savegame_CarryCurrentInfoToNextLevel(int32_t src_level, int32_t dst_level);
void Savegame_PersistGameToCurrentInfo(int level_num);

void Savegame_PreprocessItems(void);
