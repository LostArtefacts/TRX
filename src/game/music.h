#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

// Initializes music mixer.
bool Music_Init(void);

// Shuts music mixer down.
void Music_Shutdown(void);

// Stops playing current track and plays a single track. Once playback is done,
// if there is an active looped track, the playback resumes from the start of
// the looped track.
bool Music_Play(MUSIC_TRACK_ID track);

// Stops playing current track and plays a single track. Activates looped
// playback for the chosen track.
bool Music_PlayLooped(MUSIC_TRACK_ID track);

// Stops any music, whether looped or active speech.
void Music_Stop(void);

// Stops the provided single track and restarts the looped track if applicable.
void Music_StopTrack(MUSIC_TRACK_ID track);

// Gets the game volume.
int16_t Music_GetVolume(void);

// Sets the game volume. Value can be 0-10.
void Music_SetVolume(int16_t volume);

// Gets the minimum possible game volume.
int16_t Music_GetMinVolume(void);

// Gets the maximum possible game volume.
int16_t Music_GetMaxVolume(void);

// Pauses the music.
void Music_Pause(void);

// Unpauses the music.
void Music_Unpause(void);

// Returns the currently playing track. Ignores looped tracks.
MUSIC_TRACK_ID Music_GetCurrentTrack(void);

// Returns the last played track. Ignores looped tracks.
MUSIC_TRACK_ID Music_GetLastPlayedTrack(void);

// Returns the looped track.
MUSIC_TRACK_ID Music_GetCurrentLoopedTrack(void);

// Get the duration of the current stream in seconds.
double Music_GetDuration(void);

// Get the current timestamp of the current stream in seconds.
double Music_GetTimestamp(void);

// Seek to timestamp of current stream.
bool Music_SeekTimestamp(double timestamp);
