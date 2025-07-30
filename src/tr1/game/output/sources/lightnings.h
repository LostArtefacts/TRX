#pragma once

#include "game/output/types.h"

#include <libtrx/game/math/types.h>

void OutputSource_Lightnings_Init(void);
void OutputSource_Lightnings_Shutdown(void);

void OutputSource_Lightnings_StageSegment(const LIGHTNING_SEGMENT *segment);
