#pragma once

#include "global/types.h"

#include <stdint.h>

// Status returned upon every logical frame by the control routine.
// - Set .end = false to keep the phase loop spinning.
// - Set .end = true to end the current phase.
//
// To continue executing current game sequence, .command.action member should
// be set to GF_CONTINUE_SEQUENCE. To break out of the current sequence and
// switch to a different game flow action, .command.action should be set to
// the action to run.
//
// It does not make sense to return both .end = false and .command.
typedef struct {
    bool end;
    GAMEFLOW_COMMAND command;
} PHASE_CONTROL;

typedef enum {
    PHASE_NULL,
    PHASE_GAME,
    PHASE_DEMO,
    PHASE_CUTSCENE,
    PHASE_PAUSE,
    PHASE_PICTURE,
    PHASE_STATS,
    PHASE_INVENTORY,
    PHASE_PHOTO_MODE,
} PHASE;

typedef struct {
    void (*start)(void *arg);
    void (*end)();
    PHASE_CONTROL (*control)(int32_t nframes);
    void (*draw)(void);
    int32_t (*wait)(void);
} PHASER;

PHASE Phase_Get(void);
void Phase_Set(PHASE phase, void *arg);
GAMEFLOW_COMMAND Phase_Run();
