#pragma once

#include "game/phase/phase.h"

typedef struct {
    PHASE phase_to_return_to;
    void *phase_arg;
} PHASE_PHOTO_MODE_ARGS;

extern PHASER g_PhotoModePhaser;
