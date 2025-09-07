#pragma once

#include "global/types.h"

#include <libtrx/game/sound.h>

#include <stdint.h>

void Sound_UpdateEffects(void);
void Sound_ResetEffects(void);
int32_t Sound_GetMaxSamples(void);
void Sound_ResetAmbient(void);
