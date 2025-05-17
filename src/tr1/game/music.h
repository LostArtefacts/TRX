#pragma once

#include "global/types.h"

#include <libtrx/game/music.h>

// Initializes music mixer.
bool Music_Init(void);

// Shuts music mixer down.
void Music_Shutdown(void);

// Mutes the game music. Doesn't change the music volume.
void Music_Mute(void);

// Unmutes the game music. Doesn't change the music volume.
void Music_Unmute(void);

// Returns the last played track. Ignores looped tracks.
MUSIC_TRACK_ID Music_GetLastPlayedTrack(void);

// Get the current timestamp of the current stream in seconds.
double Music_GetTimestamp(void);

// Seek to timestamp of current stream.
bool Music_SeekTimestamp(double timestamp);
