#ifndef T1M_SPECIFIC_SNDPC_H
#define T1M_SPECIFIC_SNDPC_H

#include <stdint.h>

// clang-format off
#define S_SoundStopAllSamples   ((void          (*)())0x00438CC0)
// clang-format on

int32_t SoundInit();
int32_t CDPlay(int16_t track_id);
int32_t CDPlayLooped();
int32_t S_CDPlay(int16_t track);
int32_t S_CDStop();
int32_t S_StartSyncedAudio(int16_t track);
void S_CDLoop();
void S_CDVolume(int16_t volume);

void T1MInjectSpecificSndPC();

#endif
