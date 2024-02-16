#pragma once

#include <stdint.h>

typedef enum PHASE {
    PHASE_UNKNOWN,
    PHASE_GAME,
} PHASE;

typedef struct PHASER {
    void (*start)();
    void (*end)();
    int32_t (*control)(int32_t nframes);
    int32_t (*draw)(void);
} PHASER;

void Phase_Set(PHASE phase);
int32_t Phase_Control(int32_t nframes);
int32_t Phase_Draw(void);
