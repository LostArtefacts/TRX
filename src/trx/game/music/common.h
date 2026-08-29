#pragma once

#include <trx/core/result.h>
#include <trx/game/music/enum.h>
#include <trx/game/music/ids.h>
#include <trx/game/music/types.h>

#include <stdint.h>

#define MUSIC_MAX_OVERLAY_TRACKS 3

typedef struct {
    MUSIC_SLOT track_id;
    MUSIC_PLAY_MODE mode;
    double timestamp;
} MUSIC_STREAM_STATE;

// Selects a backend and opens the audio device, reporting a device that will
// not open. A headless run creates the backend without a device.
RESULT Music_Init(void);

// Stops playing current track and plays a single track.
//
// MPM_ONCE:
//   Plays the track once. Once playback is done, if there is an active looped
//   track, the playback resumes from the start of the looped track.
// MPM_LOOP:
//   Activates looped playback for the chosen track.
// MPM_NO_REPEAT:
//   A track with this play mode will not trigger in succession.
// MPM_DELAY:
//   A track does not get played and instead is only marked for later playback.
//   The track to play is available with Music_GetDelayedTrack().
// MPM_OVERLAY:
//   Plays a non-looping track without interrupting active background music.
// Returns the stream slot the track plays in - the main stream is slot 0, the
// overlays slots 1.. - or -1 when it does not play, which includes a track
// marked for later (delay) or a deferred ambient.
int32_t Music_PlayBySlot(MUSIC_SLOT track, MUSIC_PLAY_MODE mode);

// Plays a track from the given timestamp in seconds, as Music_PlayBySlot does
// otherwise. The track is seeked before it becomes audible, so its beginning is
// never heard. A negative timestamp plays the track from where it would
// normally start.
int32_t Music_PlayAtBySlot(
    MUSIC_SLOT track, MUSIC_PLAY_MODE mode, double timestamp);

// Stops the provided single track and restarts the looped track if applicable.
void Music_StopTrackBySlot(MUSIC_SLOT track);

// Play a music track with a semantical ID that will get mapped to a specific
// music track slot depending on the game. Returns the stream slot, as
// Music_PlayBySlot does, or -1.
int32_t Music_Play(MUSIC_ID track, MUSIC_PLAY_MODE mode);

// Returns true when the active backend can play the given direct track ID.
bool Music_IsTrackAvailableBySlot(MUSIC_SLOT track);

// Returns one past the largest direct track ID worth probing on the active
// backend, or 0 if no backend is available.
int32_t Music_GetTrackLimit(void);

// Resolves a track to its file path, freshly allocated for the caller to free,
// or nullptr when there is no file for it: a CD-audio backend, or a track the
// level does not carry.
char *Music_GetTrackPath(MUSIC_SLOT track);

// How long a track runs, in seconds, as its file says, or a negative value
// where nothing can answer: no backend, no file behind the track, or a
// container that does not carry a duration. The file is read the first time a
// track is asked about and the answer is kept.
double Music_GetTrackDuration(MUSIC_SLOT track);

// Stops all music streams, including looped, active, and overlay tracks.
void Music_Stop(void);

// Pauses the music.
void Music_Pause(void);

// Unpauses the music.
void Music_Unpause(void);

// Get the current timestamp of the current stream in seconds.
double Music_GetTimestamp(void);

// Seeks the current stream to the given timestamp, reporting a state where no
// music plays.
RESULT Music_SeekTimestamp(double timestamp);

// Seeks to the given timestamp if the drift is too big.
RESULT Music_SyncTimestamp(double timestamp);

// Play the current track at the given rate, so a sped-up cutscene carries its
// music with it instead of seeking away from it.
RESULT Music_SetSpeed(double speed);

// Returns the number of currently active serializable streams.
int32_t Music_GetStreamCount(void);

// Returns stream state by active index [0..Music_GetStreamCount()).
bool Music_GetStreamState(int32_t index, MUSIC_STREAM_STATE *state);

// The number of stream slots: the main stream, then the overlay slots. Unlike
// Music_GetStreamCount, this is fixed and addresses a slot whether it is active
// or not: slot 0 is the main stream, slots 1.. are the overlays.
int32_t Music_GetStreamSlotCount(void);

// Fills state for a stream slot. Returns false when the slot is inactive.
bool Music_GetStreamSlotState(int32_t slot, MUSIC_STREAM_STATE *state);

// Stops the stream in a slot. The main slot resumes the deferred ambient loop,
// as Music_StopTrackBySlot does; an overlay slot just closes.
void Music_StopStream(int32_t slot);

// Pauses or resumes the stream in a slot.
void Music_PauseStream(int32_t slot);
void Music_UnpauseStream(int32_t slot);

// Seeks the stream in a slot to the given timestamp, reporting a slot that
// holds no music.
RESULT Music_SeekStream(int32_t slot, double timestamp);

// Seeks timestamp for the active stream that matches track and mode.
RESULT Music_SeekTrackTimestamp(
    MUSIC_SLOT track, MUSIC_PLAY_MODE mode, double timestamp);

// Returns the delayed track. Ignores looped tracks.
MUSIC_SLOT Music_GetDelayedTrack(void);

// Returns the currently playing track. Includes looped music.
MUSIC_SLOT Music_GetCurrentPlayingTrack(void);

// Returns the looped track.
MUSIC_SLOT Music_GetCurrentLoopedTrack(void);

// Sets the game volume.
void Music_SetVolume(float volume);

// Resets all track trigger state.
void Music_ResetTrackStates(void);

// Returns the accumulated trigger state for the given track.
MUSIC_TRACK_STATE *Music_GetTrackState(MUSIC_SLOT track_id);

// Applies a trigger to the track, playing or stopping it per the OG rules.
void Music_Trigger(MUSIC_SLOT track_id, const MUSIC_TRIGGER *trigger);
