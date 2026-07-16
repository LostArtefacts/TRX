// A soundtrack with exactly one track in it. Playing anything else fails, which
// is how the engine reports a track that is not there.

#include "fake_engine_music.h"

#include <trx/core/memory.h>
#include <trx/game/music/common.h>
#include <trx/game/music/ids.h>

#include <stdbool.h>

FAKE_MUSIC_CALLS g_FakeMusicCalls;
static MUSIC_ID m_Playing = MX_INACTIVE;
static MUSIC_ID m_Looped = MX_INACTIVE;

typedef struct {
    bool active;
    int32_t track_id;
    int32_t mode;
    double timestamp;
} FAKE_SLOT;

static FAKE_SLOT m_Slots[FAKE_MUSIC_SLOT_COUNT];

bool Music_Play_Direct(const MUSIC_ID track, const MUSIC_PLAY_MODE mode)
{
    g_FakeMusicCalls.play_count++;
    g_FakeMusicCalls.last_track = track;
    g_FakeMusicCalls.last_mode = mode;
    if (track != FAKE_MUSIC_TRACK) {
        return false;
    }
    m_Playing = track;
    return true;
}

MUSIC_ID Music_GetCurrentPlayingTrack(void)
{
    return m_Playing;
}

MUSIC_ID Music_GetCurrentLoopedTrack(void)
{
    return m_Looped;
}

bool Music_IsTrackAvailable_Direct(const MUSIC_ID track)
{
    return track == FAKE_MUSIC_TRACK;
}

int32_t Music_GetTrackLimit(void)
{
    return FAKE_MUSIC_TRACK + 1;
}

char *Music_GetTrackPath(const MUSIC_ID track)
{
    if (track != FAKE_MUSIC_TRACK) {
        return nullptr;
    }
    return Memory_DupStr(FAKE_MUSIC_TRACK_PATH);
}

int32_t Music_GetStreamSlotCount(void)
{
    return FAKE_MUSIC_SLOT_COUNT;
}

bool Music_GetStreamSlotState(
    const int32_t slot, MUSIC_STREAM_STATE *const state)
{
    if (slot < 0 || slot >= FAKE_MUSIC_SLOT_COUNT || !m_Slots[slot].active) {
        return false;
    }
    state->track_id = m_Slots[slot].track_id;
    state->mode = m_Slots[slot].mode;
    state->timestamp = m_Slots[slot].timestamp;
    return true;
}

void Music_StopStream(const int32_t slot)
{
    g_FakeMusicCalls.stream_stop_count++;
    g_FakeMusicCalls.stream_stop_slot = slot;
    if (slot >= 0 && slot < FAKE_MUSIC_SLOT_COUNT) {
        m_Slots[slot].active = false;
    }
}

void Music_PauseStream(const int32_t slot)
{
    g_FakeMusicCalls.stream_pause_count++;
    g_FakeMusicCalls.stream_pause_slot = slot;
}

void Music_UnpauseStream(const int32_t slot)
{
    g_FakeMusicCalls.stream_unpause_count++;
    g_FakeMusicCalls.stream_unpause_slot = slot;
}

bool Music_SeekStream(const int32_t slot, const double timestamp)
{
    g_FakeMusicCalls.stream_seek_count++;
    g_FakeMusicCalls.stream_seek_slot = slot;
    g_FakeMusicCalls.stream_seek_ts = timestamp;
    if (slot < 0 || slot >= FAKE_MUSIC_SLOT_COUNT || !m_Slots[slot].active) {
        return false;
    }
    return true;
}

void Music_Pause(void)
{
    g_FakeMusicCalls.pause_count++;
}

void Music_Unpause(void)
{
    g_FakeMusicCalls.unpause_count++;
}

void Music_Stop(void)
{
    g_FakeMusicCalls.stop_count++;
    m_Playing = MX_INACTIVE;
}

void FakeMusic_Reset(void)
{
    g_FakeMusicCalls = (FAKE_MUSIC_CALLS) {};
    m_Playing = MX_INACTIVE;
    m_Looped = MX_INACTIVE;
    for (int32_t i = 0; i < FAKE_MUSIC_SLOT_COUNT; i++) {
        m_Slots[i] = (FAKE_SLOT) {};
    }
}

void FakeMusic_SetStream(
    const int32_t slot, const int32_t track, const int32_t mode,
    const double ts)
{
    if (slot < 0 || slot >= FAKE_MUSIC_SLOT_COUNT) {
        return;
    }
    m_Slots[slot] = (FAKE_SLOT) {
        .active = true,
        .track_id = track,
        .mode = mode,
        .timestamp = ts,
    };
}

void FakeMusic_SetPlaying(const int32_t track)
{
    m_Playing = (MUSIC_ID)track;
}

void FakeMusic_SetLooped(const int32_t track)
{
    m_Looped = (MUSIC_ID)track;
}
