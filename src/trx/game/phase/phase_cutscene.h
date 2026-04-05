#pragma once

#include <trx/game/phase/types.h>

typedef struct {
    int32_t level_num;
    bool cross_fade_in;
} PHASE_CUTSCENE_ARGS;

PHASE *Phase_Cutscene_Create(PHASE_CUTSCENE_ARGS args);
void Phase_Cutscene_Destroy(PHASE *phase);
