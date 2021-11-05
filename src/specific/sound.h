#ifndef T1M_SPECIFIC_SOUND_H
#define T1M_SPECIFIC_SOUND_H

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool S_Sound_Init();
void S_Sound_LoadSamples(char **sample_pointers, int32_t num_samples);

void *S_Sound_PlaySample(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan);
void *S_Sound_PlaySampleLooped(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan);
bool S_Sound_SampleIsPlaying(void *handle);
void S_Sound_StopAllSamples();
void S_Sound_StopSample(void *handle);
void S_Sound_SetPanAndVolume(void *handle, int16_t pan, int16_t volume);

#endif
