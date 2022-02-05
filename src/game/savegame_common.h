#pragma once

#include <stdint.h>

typedef enum SAVEGAME_FORMAT {
    SAVEGAME_FORMAT_LEGACY = 1,
    SAVEGAME_FORMAT_BSON = 2,
} SAVEGAME_FORMAT;

typedef struct SAVEGAME_INFO {
    SAVEGAME_FORMAT format;
    int32_t counter;
    int32_t level_num;
    char *level_title;
} SAVEGAME_INFO;
