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
bool Music_Play_Direct(MUSIC_ID track, MUSIC_PLAY_MODE mode);

// Stops the provided single track and restarts the looped track if applicable.
void Music_StopTrack_Direct(MUSIC_ID track);

// Play a music track with a semantical ID that will get mapped to a specific
// music track slot depending on the game.
bool Music_Play(MUSIC_TRX_ID track, MUSIC_PLAY_MODE mode);

// Stops any music, whether looped or active speech.
void Music_Stop(void);

// Pauses the music.
void Music_Pause(void);

// Unpauses the music.
void Music_Unpause(void);

// Get the current timestamp of the current stream in seconds.
double Music_GetTimestamp(void);

// Seek to timestamp of current stream.
bool Music_SeekTimestamp(double timestamp);

// Seeks to the given timestamp if the drift is too big.
bool Music_SyncTimestamp(double timestamp);

// Returns the delayed track. Ignores looped tracks.
MUSIC_ID Music_GetDelayedTrack(void);

// Returns the currently playing track. Includes looped music.
MUSIC_ID Music_GetCurrentPlayingTrack(void);

// Returns the looped track.
MUSIC_ID Music_GetCurrentLoopedTrack(void);

// Sets the game volume.
void Music_SetVolume(float volume);

// Resets all track trigger mask flags.
void Music_ResetTrackFlags(void);

// Returns trigger mask flags for the given track.
uint16_t Music_GetTrackFlags(MUSIC_ID track_id);

// Sets the trigger mask flags for the given track.
void Music_SetTrackFlags(MUSIC_ID track_id, uint16_t flags);

MUSIC_ID Music_ConvertLegacyTrack(MUSIC_ID track_id);
