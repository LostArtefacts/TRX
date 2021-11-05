#ifndef T1M_SPECIFIC_MUSIC_H
#define T1M_SPECIFIC_MUSIC_H

#include <stdbool.h>
#include <stdint.h>

#include "global/types.h"

int32_t S_Music_Init();
int32_t S_Music_PlayImpl(int16_t track_id);
void S_Music_PlayLooped();
int32_t S_Music_Play(int16_t track);
int32_t S_Music_Stop();
void S_Music_Loop();
void S_Music_AdjustVolume(int16_t volume);
void S_Music_Pause();
void S_Music_Unpause();

#endif
