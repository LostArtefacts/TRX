#pragma once

#include <trx/game/phase/types.h>

GF_COMMAND PhaseExecutor_Run(PHASE *phase);

// Starts the fade that runs before the game exits and reports whether the fade
// still has frames to draw. A render loop of its own, such as an FMV, calls
// this every frame and gives up the loop once it reports false.
bool PhaseExecutor_BeginExit(void);

// Returns how black the exit fade is, from 0 for the picture to 1 for black.
float PhaseExecutor_GetExitFadeOpacity(void);

PHASE *PhaseExecutor_GetOuterPhase(void);
