#pragma once

#include "game/game_flow/types.h"
#include "global/types.h"

#include <libtrx/filesystem.h>
#include <libtrx/game/savegame.h>

#include <stddef.h>

void Savegame_Init(void);
void Savegame_Shutdown(void);

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

void Savegame_ProcessItemsBeforeSave(void);
void Savegame_ProcessItemsBeforeLoad(void);

void S_SaveGame(MYFILE *fp);
void S_LoadGame(MYFILE *fp);

void Savegame_SetDefaultStats(const GF_LEVEL *level, STATS_COMMON stats);
STATS_COMMON Savegame_GetDefaultStats(const GF_LEVEL *level);
