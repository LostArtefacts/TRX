#pragma once

#include "game/phase/phase.h"

typedef struct {
    const char *path;
    double display_time;
} PHASE_PICTURE_DATA;

extern PHASER g_PicturePhaser;
