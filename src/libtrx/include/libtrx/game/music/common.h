#pragma once

#include "./enum.h"
#include "./ids.h"

#include <stdint.h>

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
extern void Music_Stop(void);

// Pauses the music.
extern void Music_Pause(void);

// Unpauses the music.
extern void Music_Unpause(void);

void Music_ResetTrackFlags(void);
uint16_t Music_GetTrackFlags(int32_t track_idx);
void Music_SetTrackFlags(int32_t track, uint16_t flags);
