#pragma once

#include "game/phase/phase.h"
#include "global/types.h"

#include <stdint.h>

typedef struct PHASE_STATS_DATA {
    int32_t level_num;
} PHASE_STATS_DATA;

extern PHASER g_StatsPhaser;
