#pragma once

#include <trx/core/result.h>

#include <SDL2/SDL_audio.h>
#include <libavutil/samplefmt.h>
#include <stddef.h>
#include <stdint.h>

#define AUDIO_MAX_SAMPLES 1000
#define AUDIO_MAX_ACTIVE_SAMPLES 50
#define AUDIO_MAX_ACTIVE_STREAMS 10
#define AUDIO_DRIFT_THRESHOLD 0.2
#define AUDIO_NO_SOUND (-1)

// Opens the audio device and starts the mixer, reporting a device that will
// not open. A second call while the device is open only adds a reference.
RESULT Audio_Init(void);
RESULT Audio_Shutdown(void);
bool Audio_ShouldSkipSDLQuitAudio(void);

void Audio_Mute(void);
void Audio_Unmute(void);
bool Audio_IsMuted(void);

RESULT Audio_Stream_Pause(int32_t sound_id);
RESULT Audio_Stream_Unpause(int32_t sound_id);
RESULT Audio_Stream_SetPaused(int32_t sound_id, bool is_paused);
// Creates a stream and returns its id, reporting audio that cannot be read
// and a state where every stream is already in use. The stream is paused, so
// that the caller can seek it and set its volume before anything is heard; call
// Audio_Stream_Unpause to start it.
RESULT Audio_Stream_CreateFromFile(const char *path, int32_t *out_sound_id);
RESULT Audio_Stream_CreateFromMemory(
    uint8_t *data, size_t size, int32_t *out_sound_id);
RESULT Audio_Stream_Close(int32_t sound_id);
bool Audio_Stream_IsLooped(int32_t sound_id);
RESULT Audio_Stream_SetVolume(int32_t sound_id, float volume);

// Play the stream faster or slower, pitching it with the rate the way a tape
// does. Timestamps stay in the source timeline.
RESULT Audio_Stream_SetSpeed(int32_t sound_id, double speed);
RESULT Audio_Stream_SetIsLooped(int32_t sound_id, bool is_looped);

// Synchronizes the stream to the given timestamp, and seeks it where the
// drift is too large. A stream that is already synchronized is no fault and
// does not move.
RESULT Audio_Stream_SyncTimestamp(int32_t sound_id, double timestamp);

RESULT Audio_Stream_SetFinishCallback(
    int32_t sound_id, void (*callback)(int32_t sound_id, void *user_data),
    void *user_data);
double Audio_Stream_GetTimestamp(int32_t sound_id);
double Audio_Stream_GetDuration(int32_t sound_id);
RESULT Audio_Stream_SeekTimestamp(int32_t sound_id, double timestamp);
RESULT Audio_Stream_SetStartTimestamp(int32_t sound_id, double timestamp);
RESULT Audio_Stream_SetStopTimestamp(int32_t sound_id, double timestamp);

// Stores the data of a sample for later playback, reporting data it cannot
// use and a slot that is already in use.
RESULT Audio_Sample_Load(int32_t sample_num, const char *content, size_t size);

// Returns whether a sample slot holds audio to play.
bool Audio_Sample_IsLoaded(int32_t sample_num);
RESULT Audio_Sample_Unload(int32_t sample_id);
RESULT Audio_Sample_UnloadAll(void);

int32_t Audio_Sample_Play(
    int32_t sample_id, int32_t volume, float pitch, int32_t pan,
    bool is_looped);
bool Audio_Sample_IsPlaying(int32_t sound_id);
RESULT Audio_Sample_Pause(int32_t sound_id);
RESULT Audio_Sample_PauseAll(void);
RESULT Audio_Sample_Unpause(int32_t sound_id);
RESULT Audio_Sample_UnpauseAll(void);
RESULT Audio_Sample_Close(int32_t sound_id);
RESULT Audio_Sample_CloseAll(void);
RESULT Audio_Sample_SetPan(int32_t sound_id, int32_t pan);
RESULT Audio_Sample_SetVolume(int32_t sound_id, int32_t volume);
RESULT Audio_Sample_SetPitch(int32_t sound_id, float pan);

void Audio_SetReverbType(uint8_t reverb_type);
uint8_t Audio_GetReverbType(void);
