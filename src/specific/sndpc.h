#ifndef T1M_SPECIFIC_SNDPC_H
#define T1M_SPECIFIC_SNDPC_H

#include <stdint.h>

// clang-format off
#define CDStop                  ((void          (*)())0x00437F80)
#define CDPlay                  ((void          (*)(int16_t track))0x00437FB0)
#define S_CDLoop                ((void          (*)())0x004380B0)
#define S_CDStop                ((void          (*)())0x00438E40)
#define S_SoundStopAllSamples   ((void          (*)())0x00438CC0)
#define S_StartSyncedAudio      ((void          (*)(int32_t num))0x00439030)
#define SoundInit               ((int32_t       (*)())0x00437E00)
// clang-format on

void S_CDPlay(int16_t track);
void S_CDVolume(int16_t volume);

void T1MInjectSpecificSndPC();

#endif
