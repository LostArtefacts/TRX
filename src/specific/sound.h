#ifndef T1M_SPECIFIC_SOUND_H
#define T1M_SPECIFIC_SOUND_H

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool S_Sound_Init();
void S_Sound_LoadSamples(char **sample_pointers, int32_t num_samples);
SAMPLE_DATA *S_Sound_LoadSample(char *content);
int32_t S_Sound_MakeSample(SAMPLE_DATA *sample_data);
void *S_Sound_PlaySampleImpl(
    int32_t sample_id, int32_t volume, int16_t pitch, uint16_t pan,
    int8_t loop);

void *S_Sound_PlaySample(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan);
void *S_Sound_PlaySampleLooped(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan);
int32_t S_Sound_SampleIsPlaying(void *handle);
void S_Sound_StopAllSamples();
void S_Sound_StopSample(void *handle);
void S_Sound_SetPanAndVolume(void *handle, int16_t pan, int16_t volume);

#endif
