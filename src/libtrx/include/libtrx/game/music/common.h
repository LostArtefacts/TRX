#pragma once

#include "./enum.h"
#include "./ids.h"

#include <stdint.h>

bool Music_Init(void);
void Music_Shutdown(void);

// Stops playing current track and plays a single track.
//
// MPM_ALWAYS:
//   Plays the track once. Once playback is done, if there is an active looped
//   track, the playback resumes from the start of the looped track.
// MPM_LOOPED:
//   Activates looped playback for the chosen track.
// MPM_TRACKED:
//   A track with this play mode will not trigger in succession.
// MPM_DELAYED:
//   A track does not get played and instead is only marked for later playback.
//   The track to play is available with Music_GetDelayedTrack().
bool Music_Play(MUSIC_TRACK_ID track, MUSIC_PLAY_MODE mode);

// Stops any music, whether looped or active speech.
void Music_Stop(void);

// Stops the provided single track and restarts the looped track if applicable.
void Music_StopTrack(MUSIC_TRACK_ID track);

// Pauses the music.
void Music_Pause(void);

// Unpauses the music.
void Music_Unpause(void);

// Get the current timestamp of the current stream in seconds.
double Music_GetTimestamp(void);

// Seek to timestamp of current stream.
bool Music_SeekTimestamp(double timestamp);

// Returns the delayed track. Ignores looped tracks.
MUSIC_TRACK_ID Music_GetDelayedTrack(void);

// Returns the currently playing track. Includes looped music.
MUSIC_TRACK_ID Music_GetCurrentPlayingTrack(void);

// Returns the looped track.
MUSIC_TRACK_ID Music_GetCurrentLoopedTrack(void);

// Gets the minimum possible game volume.
int32_t Music_GetMinVolume(void);

// Gets the maximum possible game volume.
int32_t Music_GetMaxVolume(void);

// Sets the game volume.
void Music_SetVolume(int32_t volume);

// Mutes the game music. Doesn't change the music volume.
void Music_Mute(void);

// Unmutes the game music. Doesn't change the music volume.
void Music_Unmute(void);

// Resets all track trigger mask flags.
void Music_ResetTrackFlags(void);

// Returns trigger mask flags for the given track.
uint16_t Music_GetTrackFlags(int32_t track_idx);

// Sets the trigger mask flags for the given track.
void Music_SetTrackFlags(int32_t track, uint16_t flags);
