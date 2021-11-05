#ifndef T1M_SPECIFIC_SNDPC_H
#define T1M_SPECIFIC_SNDPC_H

#include <stdbool.h>
#include <stdint.h>

#include "global/types.h"

int32_t MusicInit();
int32_t MusicPlay(int16_t track_id);
void MusicPlayLooped();
int32_t S_MusicPlay(int16_t track);
int32_t S_MusicStop();
void S_MusicLoop();
void S_MusicVolume(int16_t volume);
void S_MusicPause();
void S_MusicUnpause();

#endif
