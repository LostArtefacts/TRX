#pragma once

#include <trx/game/phase/types.h>

GF_COMMAND PhaseExecutor_Run(PHASE *phase);

// Starts the fade that runs before the game exits and reports whether the fade
// still has frames to draw. A render loop of its own, such as an FMV, calls
// this every frame and gives up the loop once it reports false.
bool PhaseExecutor_BeginExit(void);

PHASE *PhaseExecutor_GetOuterPhase(void);

// Makes the next phase draw its first frame until the graphics card keeps up
// before the phase moves on. Call it after the card has had little to draw,
// such as after an FMV.
void PhaseExecutor_WarmUpNextPhase(void);
