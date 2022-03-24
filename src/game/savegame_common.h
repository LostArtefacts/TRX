#pragma once

#include <stdint.h>

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
} SAVEGAME_INFO;

void Savegame_ResetStartInfo(int level_num);
void Savegame_ApplyLogicToStartInfo(int level_num);
void Savegame_ResetCurrentInfo(int level_num);
void Savegame_CarryCurrentInfoToStartInfo(int32_t src_level, int32_t dst_level);
void Savegame_PersistGameToCurrentInfo(int level_num);

#ifdef SAVEGAME_IMPL
void Savegame_SetCurrentPosition(int level_num);
#endif
