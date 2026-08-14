#pragma once

#include <trx/core/file.h>
#include <trx/core/result.h>
#include <trx/game/game_flow/types.h>

typedef enum {
    LEVEL_FORMAT_PROBE_MINIMAL,
    LEVEL_FORMAT_PROBE_STATS,
} LEVEL_FORMAT_PROBE_MODE;

typedef enum {
    LEVEL_FORMAT_LAYOUT_UNKNOWN = -1,
    LEVEL_FORMAT_LAYOUT_TR1X,
    LEVEL_FORMAT_LAYOUT_TR1,
    LEVEL_FORMAT_LAYOUT_TR1_DEMO_PC,
    LEVEL_FORMAT_LAYOUT_TR2X,
    LEVEL_FORMAT_LAYOUT_TR2,
    LEVEL_FORMAT_LAYOUT_TR3,
    LEVEL_FORMAT_LAYOUT_TR3X,
    LEVEL_FORMAT_LAYOUT_TR4,
    LEVEL_FORMAT_LAYOUT_NUMBER_OF,
} LEVEL_FORMAT_LAYOUT;

typedef struct LEVEL_FORMAT_LOADER {
    int32_t game_version;
    LEVEL_FORMAT_LAYOUT layout;
    bool (*probe)(
        const struct LEVEL_FORMAT_LOADER *, TRX_FILE *file,
        LEVEL_FORMAT_PROBE_MODE mode);
    RESULT (*load)(const struct LEVEL_FORMAT_LOADER *, TRX_FILE *file);
} LEVEL_FORMAT_LOADER;

LEVEL_FORMAT_LAYOUT Level_Format_GuessLayout(TRX_FILE *file);
const LEVEL_FORMAT_LOADER *Level_Format_GuessLoader(TRX_FILE *file);
// Reads a level from the path the game flow gives it, reporting a level that
// cannot be opened or is in no format TRX knows.
RESULT Level_Format_LoadFromFile(
    const GF_LEVEL *level, const LEVEL_FORMAT_LOADER **out_loader);
