#pragma once

#include "game/game_flow/types.h"
#include "global/types.h"

#include <libtrx/game/savegame.h>

#include <stddef.h>

void Savegame_Init(void);
void Savegame_Shutdown(void);

void Savegame_InitCurrentInfo(void);

void Savegame_ResetCurrentInfo(const GF_LEVEL *level);
START_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *level);
void Savegame_CarryCurrentInfoToNextLevel(
    const GF_LEVEL *src_level, const GF_LEVEL *dst_level);
void Savegame_ApplyLogicToCurrentInfo(const GF_LEVEL *level);
void Savegame_PersistGameToCurrentInfo(const GF_LEVEL *level);
void CreateSaveGameInfo(void);
void ExtractSaveGameInfo(void);

void GetSavedGamesList(REQUEST_INFO *req);
bool S_FrontEndCheck(void);
int32_t S_SaveGame(int32_t slot_num);
int32_t S_LoadGame(int32_t slot_num);
