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
bool Music_Play(int16_t track);

// Stops playing current track and plays a single track. Activates looped
// playback for the chosen track.
bool Music_PlayLooped(int16_t track);

// Stops any music, whether looped or active speech.
void Music_Stop(void);

// Stops the provided single track and restarts the looped track if applicable.
void Music_StopTrack(int16_t track);

// Sets the game volume. Value can be 0-255.
void Music_SetVolume(int16_t volume);

// Pauses the music.
void Music_Pause(void);

// Unpauses the music.
void Music_Unpause(void);

// Returns the currently playing track. Ignores looped tracks.
MUSIC_TRACK_ID Music_CurrentTrack(void);

// Returns the last played track. Ignores looped tracks.
MUSIC_TRACK_ID Music_LastPlayedTrack(void);

// Returns the looped track.
MUSIC_TRACK_ID Music_CurrentTrackLooped(void);

// Get timestamp of current stream.
int64_t Music_GetTimestamp(void);

// Seek to timestamp of current stream.
bool Music_SeekTimestamp(int64_t timestamp);
