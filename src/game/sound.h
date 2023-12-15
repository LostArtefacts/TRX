#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool Sound_Init(void);
void Sound_Shutdown(void);
bool Sound_Effect(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags);
bool Sound_StopEffect(int32_t sfx_num, PHD_3DPOS *pos);
void Sound_UpdateEffects(void);
void Sound_ResetEffects(void);
void Sound_StopAmbientSounds(void);
void Sound_PauseAll(void);
void Sound_UnpauseAll(void);
void Sound_StopAllSamples(void);
int8_t Sound_GetMasterVolume(void);
void Sound_SetMasterVolume(int8_t volume);
int32_t Sound_GetMinVolume(void);
int32_t Sound_GetMaxVolume(void);
void Sound_LoadSamples(
    size_t num_samples, const char **sample_pointers, size_t *sizes);
void Sound_ResetAmbient(void);
