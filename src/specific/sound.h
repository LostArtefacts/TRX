#ifndef T1M_SPECIFIC_SOUND_H
#define T1M_SPECIFIC_SOUND_H

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool SoundInit();
void SoundLoadSamples(char **sample_pointers, int32_t num_samples);
SAMPLE_DATA *SoundLoadSample(char *content);
int32_t SoundMakeSample(SAMPLE_DATA *sample_data);
void *SoundPlaySample(
    int32_t sample_id, int32_t volume, int16_t pitch, uint16_t pan,
    int8_t loop);

void *S_SoundPlaySample(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan);
void *S_SoundPlaySampleLooped(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan);
int32_t S_SoundSampleIsPlaying(void *handle);
void S_SoundStopAllSamples();
void S_SoundStopSample(void *handle);
void S_SoundSetPanAndVolume(void *handle, int16_t pan, int16_t volume);

#endif
