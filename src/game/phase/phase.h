#pragma once

#include "global/types.h"

#include <stdint.h>

typedef enum PHASE {
    PHASE_NULL,
    PHASE_GAME,
    PHASE_PAUSE,
} PHASE;

typedef struct PHASER {
    void (*start)();
    void (*end)();
    int32_t (*control)(int32_t nframes);
    void (*draw)(void);
} PHASER;

void Phase_Set(PHASE phase);
GAMEFLOW_OPTION Phase_Control(int32_t nframes);
void Phase_Draw(void);
