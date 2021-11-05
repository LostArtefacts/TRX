#ifndef T1M_GAME_MUSIC_H
#define T1M_GAME_MUSIC_H

#include <stdbool.h>
#include <stdint.h>

#include "global/types.h"

int32_t Music_Init();
void Music_PlayLooped();
int32_t Music_Play(int16_t track);
int32_t Music_Stop();
void Music_Loop();
void Music_AdjustVolume(int16_t volume);
void Music_Pause();
void Music_Unpause();

#endif
