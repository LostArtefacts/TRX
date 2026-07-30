#pragma once

#include <trx/game/music/enum.h>

#include <stdint.h>

// The only track the fake soundtrack knows about.
#define FAKE_MUSIC_TRACK 5
#define FAKE_MUSIC_MISSING_TRACK 999
#define FAKE_MUSIC_TRACK_PATH "music/track05.flac"

// The fake soundtrack's stream slots: one main, then the overlays.
#define FAKE_MUSIC_SLOT_COUNT 4

// Marks a track as playing without going through Music_Play.
void FakeMusic_SetPlaying(int32_t track);

// Sets the ambient track that resumes once a one-shot finishes.
void FakeMusic_SetLooped(int32_t track);

// Activates a stream slot with a track, mode and timestamp.
void FakeMusic_SetStream(int32_t slot, int32_t track, int32_t mode, double ts);
