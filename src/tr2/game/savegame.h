#pragma once

#include "game/game_flow/types.h"

#include <libtrx/filesystem.h>
#include <libtrx/game/savegame.h>

#define SAVEGAME_CURRENT_VERSION -1

typedef enum {
    VERSION_LEGACY = -1,
} SAVEGAME_VERSION;

typedef struct {
    bool allow_load;
    bool allow_save;
    SAVEGAME_FORMAT format;
    const char *(*get_save_file_pattern_func)(void);
    bool (*fill_info_func)(MYFILE *fp, SAVEGAME_INFO *info);
    bool (*load_from_file_func)(MYFILE *fp);
    void (*save_to_file_func)(MYFILE *fp);
} SAVEGAME_STRATEGY;

void Savegame_Init(void);
void Savegame_Shutdown(void);
void Savegame_RegisterStrategy(SAVEGAME_STRATEGY strategy);

void Savegame_InitCurrentInfo(void);

void Savegame_ResetCurrentInfo(const GF_LEVEL *level);
RESUME_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *level);
void Savegame_CarryCurrentInfoToNextLevel(
    const GF_LEVEL *src_level, const GF_LEVEL *dst_level);
void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *level);
void Savegame_PersistGameToCurrentInfo(const GF_LEVEL *level);

void Savegame_ScanSavedGames(void);
void Savegame_FillAvailableSaves(REQUEST_INFO *req);
void Savegame_HighlightNewestSlot(void);
int32_t Savegame_GetLevelNumber(int32_t slot_idx);
int32_t Savegame_GetCounter(void);
int32_t Savegame_GetTotalCount(void);
SAVEGAME_VERSION Savegame_GetInitialVersion(void);
void Savegame_SetInitialVersion(SAVEGAME_VERSION version);

void Savegame_ProcessItemsBeforeSave(void);
void Savegame_ProcessItemsBeforeLoad(void);

void Savegame_SetDefaultStats(const GF_LEVEL *level, STATS_COMMON stats);
STATS_COMMON Savegame_GetDefaultStats(const GF_LEVEL *level);

#define REGISTER_SAVEGAME_STRATEGY(strategy_)                                  \
    __attribute__((__constructor__)) static void M_Register(void)              \
    {                                                                          \
        Savegame_RegisterStrategy(strategy_);                                  \
    }
