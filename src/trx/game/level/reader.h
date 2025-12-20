#pragma once

#include <trx/virtual_file.h>

// V2 level loading facilities

typedef enum {
    LEVEL_PROBE_MINIMAL,
    LEVEL_PROBE_STATS,
} LEVEL_PROBE_MODE;

typedef enum {
    LEVEL_LAYOUT_UNKNOWN = -1,
    LEVEL_LAYOUT_TR1X,
    LEVEL_LAYOUT_TR1,
    LEVEL_LAYOUT_TR1_DEMO_PC,
    LEVEL_LAYOUT_TR2X,
    LEVEL_LAYOUT_TR2,
    LEVEL_LAYOUT_TR3,
    LEVEL_LAYOUT_TR3X,
    LEVEL_LAYOUT_NUMBER_OF,
} LEVEL_LAYOUT;

typedef struct LEVEL_LOADER {
    int32_t game_version;
    LEVEL_LAYOUT layout;
    bool (*probe)(
        const struct LEVEL_LOADER *, VFILE *file, LEVEL_PROBE_MODE mode);
    bool (*load)(const struct LEVEL_LOADER *, VFILE *file);
} LEVEL_LOADER;

extern const LEVEL_LOADER *g_LevelLoaders[];

const LEVEL_LOADER *Level_GuessLoader(VFILE *file);
LEVEL_LAYOUT Level_GuessLayout(VFILE *file);
bool Level_LoadFromFile(VFILE *file);
