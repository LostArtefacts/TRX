#pragma once

#include "game/phase/phase.h"
#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct PHASE_STATS_DATA {
    int32_t level_num;
    const char *background_path;
    bool total;
    GAMEFLOW_LEVEL_TYPE level_type;
} PHASE_STATS_DATA;

extern PHASER g_StatsPhaser;
