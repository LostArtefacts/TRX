#pragma once

#include "game/phase/phase.h"

#include <stdint.h>

typedef struct {
    bool resume_existing;
    int32_t level_num;
} PHASE_CUTSCENE_ARGS;

extern PHASER g_CutscenePhaser;
