#ifndef T1M_SPECIFIC_SNDPC_H
#define T1M_SPECIFIC_SNDPC_H

#include <stdint.h>

#include "global/types.h"

// clang-format off
#define SoundLoadSample     ((SAMPLE_DATA* (*)(char *content, char *(*sample_loader)(char *)))0x00437CB0)
// clang-format on

int32_t SoundInit();
void SoundLoadSamples(char **sample_pointers, int32_t file_size);

int32_t CDPlay(int16_t track_id);
int32_t CDPlayLooped();
int32_t S_CDPlay(int16_t track);
int32_t S_CDStop();
int32_t S_StartSyncedAudio(int16_t track);
void S_CDLoop();
void S_CDVolume(int16_t volume);
int32_t S_SoundPlaySample(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan);
int32_t S_SoundPlaySampleLooped(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan);
int32_t S_SoundSampleIsPlaying(int32_t handle);
void S_SoundStopAllSamples();
void S_SoundStopSample(int32_t handle);
void S_SoundSetPanAndVolume(int32_t handle, int16_t pan, int16_t volume);

void T1MInjectSpecificSndPC();

#endif
