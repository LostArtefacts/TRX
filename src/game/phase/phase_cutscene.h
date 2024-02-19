#pragma once

#include "game/phase/phase.h"

#include <stdint.h>

typedef struct PHASE_CUTSCENE_DATA {
    int32_t level_num;
} PHASE_CUTSCENE_DATA;

extern PHASER g_CutscenePhaser;
