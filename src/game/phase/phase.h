#pragma once

#include "global/types.h"

#include <stdint.h>

typedef enum PHASE {
    PHASE_NULL,
    PHASE_GAME,
    PHASE_CUTSCENE,
    PHASE_PAUSE,
    PHASE_PICTURE,
    PHASE_STATS,
    PHASE_INVENTORY,
} PHASE;

typedef struct PHASER {
    void (*start)(void *arg);
    void (*end)();
    int32_t (*control)(int32_t nframes);
    void (*draw)(void);
    int32_t (*wait)(void);
} PHASER;

PHASE Phase_Get(void);
void Phase_Set(PHASE phase, void *arg);
GAMEFLOW_OPTION Phase_Run();
