#ifndef T1M_SPECIFIC_S_MUSIC_H
#define T1M_SPECIFIC_S_MUSIC_H

#include <stdbool.h>
#include <stdint.h>

bool S_Music_Init();
bool S_Music_Play(int16_t track);
bool S_Music_Stop();
bool S_Music_SetVolume(int16_t volume);
bool S_Music_Pause();
bool S_Music_Unpause();

#endif
