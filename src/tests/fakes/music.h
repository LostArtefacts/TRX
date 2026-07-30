#pragma once

#include <trx/game/music/enum.h>

#include <stdint.h>

// The only track the fake soundtrack knows about.
#define FAKE_MUSIC_TRACK 5
#define FAKE_MUSIC_MISSING_TRACK 999
#define FAKE_MUSIC_TRACK_PATH "music/track05.flac"

// The fake soundtrack's stream slots: one main, then the overlays.
#define FAKE_MUSIC_SLOT_COUNT 4

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t play_count;
    int32_t last_track;
    MUSIC_PLAY_MODE last_mode;
    int32_t pause_count;
    int32_t unpause_count;
    int32_t stop_count;

    int32_t stream_stop_count;
    int32_t stream_stop_slot;
    int32_t stream_pause_count;
    int32_t stream_pause_slot;
    int32_t stream_unpause_count;
    int32_t stream_unpause_slot;
    int32_t stream_seek_count;
    int32_t stream_seek_slot;
    double stream_seek_ts;
} FAKE_MUSIC_CALLS;

extern FAKE_MUSIC_CALLS g_FakeMusicCalls;

void FakeMusic_Reset(void);

// Marks a track as playing without going through Music_Play.
void FakeMusic_SetPlaying(int32_t track);

// Sets the ambient track that resumes once a one-shot finishes.
void FakeMusic_SetLooped(int32_t track);

// Activates a stream slot with a track, mode and timestamp.
void FakeMusic_SetStream(int32_t slot, int32_t track, int32_t mode, double ts);
