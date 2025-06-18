#pragma once

#include "./types.h"

GF_COMMAND PhaseExecutor_Run(PHASE *phase);

PHASE *PhaseExecutor_GetOuterPhase(void);
