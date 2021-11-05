#ifndef T1M_GAME_MUSIC_H
#define T1M_GAME_MUSIC_H

#include <stdbool.h>
#include <stdint.h>

#include "global/types.h"

bool Music_Init();
void Music_PlayLooped();
bool Music_Play(int16_t track);
bool Music_Stop();
void Music_Loop();
void Music_AdjustVolume(int16_t volume);
void Music_Pause();
void Music_Unpause();

#endif
