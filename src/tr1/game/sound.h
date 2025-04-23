#pragma once

#include "global/types.h"

#include <libtrx/game/sound.h>

#include <stddef.h>
#include <stdint.h>

bool Sound_Init(void);
void Sound_Shutdown(void);
bool Sound_StopEffect(SOUND_EFFECT_ID sfx_num, const XYZ_32 *pos);
void Sound_UpdateEffects(void);
void Sound_ResetEffects(void);
void Sound_StopAmbientSounds(void);
void Sound_LoadSamples(
    size_t num_samples, const char **sample_pointers, size_t *sizes);
int32_t Sound_GetMaxSamples(void);
void Sound_ResetAmbient(void);
