#pragma once

#include <trx/game/music/enum.h>

#include <stdint.h>

// The only track the fake soundtrack knows about. Anything else fails to play,
// which is how the engine reports a track that is not there.
#define FAKE_MUSIC_TRACK 5
#define FAKE_MUSIC_MISSING_TRACK 999

// What the surface asked the engine to do, recorded rather than performed.
typedef struct {
    int32_t play_count;
    int32_t last_track;
    MUSIC_PLAY_MODE last_mode;
    int32_t pause_count;
    int32_t unpause_count;
    int32_t stop_count;
} FAKE_MUSIC_CALLS;

extern FAKE_MUSIC_CALLS g_FakeMusicCalls;

void FakeMusic_Reset(void);

// Nothing is playing until something is.
void FakeMusic_SetPlaying(int32_t track);
