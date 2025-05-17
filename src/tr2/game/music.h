#pragma once

#include "global/types.h"

#include <libtrx/game/music.h>

bool Music_Init(void);
void Music_Shutdown(void);
void Music_Stop(void);
double Music_GetTimestamp(void);
bool Music_SeekTimestamp(double timestamp);
void Music_SetVolume(int32_t volume);
MUSIC_TRACK_ID Music_GetCurrentPlayingTrack(void);
MUSIC_TRACK_ID Music_GetCurrentLoopedTrack(void);
MUSIC_TRACK_ID Music_GetLastPlayedTrack(void);
void Music_Pause(void);
void Music_Unpause(void);
