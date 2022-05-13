#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

int32_t ControlPhase(int32_t nframes, GAMEFLOW_LEVEL_TYPE level_type);
void RefreshCamera(int16_t type, int16_t *data);

bool Control_Pause(void);
