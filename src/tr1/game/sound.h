#pragma once

#include "global/types.h"

#include <libtrx/game/sound.h>

#include <stdint.h>

bool Sound_Init(void);
void Sound_Shutdown(void);
void Sound_UpdateEffects(void);
void Sound_ResetEffects(void);
void Sound_StopAmbientSounds(void);
int32_t Sound_GetMaxSamples(void);
void Sound_ResetAmbient(void);
