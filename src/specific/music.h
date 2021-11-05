#ifndef T1M_SPECIFIC_MUSIC_H
#define T1M_SPECIFIC_MUSIC_H

#include <stdbool.h>
#include <stdint.h>

#include "global/types.h"

bool S_Music_Init();
bool S_Music_Play(int16_t track);
bool S_Music_Stop();
void S_Music_AdjustVolume(int16_t volume);
void S_Music_Pause();
void S_Music_Unpause();

#endif
