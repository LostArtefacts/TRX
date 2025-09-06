#pragma once

typedef enum {
    SFX_INVALID = -1,
#define SOUND_DEFINE(x, y) x = y,
#include "ids.def"
} SAMPLE_ID;
