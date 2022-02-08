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

void ResetStartInfo(int level_num);
void CreateStartInfo(int level_num);
void ModifyStartInfo(int level_num);
void ResetEndInfo(int level_num);
void CreateEndInfo(int level_num);
