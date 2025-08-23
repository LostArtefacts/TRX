#pragma once

#include "../../types.h"
#include "../types.h"

void OutputSource_Lightnings_Init(void);
void OutputSource_Lightnings_Shutdown(void);

void OutputSource_Lightnings_StageSegment(const LIGHTNING_SEGMENT *segment);
