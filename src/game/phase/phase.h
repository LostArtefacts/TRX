#pragma once

#include "global/types.h"

#include <stdint.h>

typedef enum PHASE {
    PHASE_NULL,
    PHASE_GAME,
    PHASE_PAUSE,
    PHASE_PICTURE,
} PHASE;

typedef struct PHASER {
    void (*start)(void *arg);
    void (*end)();
    int32_t (*control)(int32_t nframes);
    void (*draw)(void);
} PHASER;

void Phase_Set(PHASE phase, void *arg);
GAMEFLOW_OPTION Phase_Control(int32_t nframes);
void Phase_Draw(void);
