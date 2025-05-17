#pragma once

#include "global/types.h"

#include <libtrx/game/music.h>

bool Music_Init(void);
void Music_Shutdown(void);
double Music_GetTimestamp(void);
bool Music_SeekTimestamp(double timestamp);
MUSIC_TRACK_ID Music_GetLastPlayedTrack(void);
