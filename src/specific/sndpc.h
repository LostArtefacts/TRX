#ifndef T1M_SPECIFIC_SNDPC_H
#define T1M_SPECIFIC_SNDPC_H

#include <stdint.h>

int32_t SoundInit();
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
void S_SoundStopAllSamples();
void S_SoundStopSample(int32_t handle);

void T1MInjectSpecificSndPC();

#endif
