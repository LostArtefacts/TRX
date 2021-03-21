#ifndef T1M_SPECIFIC_SNDPC_H
#define T1M_SPECIFIC_SNDPC_H

#include <stdint.h>

// clang-format off
#define CDStop                  ((void          (*)())0x00437F80)
#define S_CDLoop                ((void          (*)())0x004380B0)
#define S_SoundStopAllSamples   ((void          (*)())0x00438CC0)
#define SoundInit               ((int32_t       (*)())0x00437E00)
// clang-format on

int32_t CDPlay(int16_t track_id);
int32_t S_CDPlay(int16_t track);
int32_t S_CDStop();
int32_t S_StartSyncedAudio(int16_t track);
void S_CDVolume(int16_t volume);

void T1MInjectSpecificSndPC();

#endif
