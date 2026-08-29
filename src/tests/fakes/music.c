// A soundtrack with exactly one track in it. Playing anything else fails, which
// is how the engine reports a track that is not there.

#include <fakes/music.h>

#include <harness/fake_calls.h>

#include <trx/core/memory.h>
#include <trx/game/music/common.h>
#include <trx/game/music/ids.h>

typedef struct {
    bool active;
    int32_t track_id;
    int32_t mode;
    double timestamp;
} FAKE_SLOT;

static MUSIC_SLOT m_Playing = MX_INACTIVE;
static MUSIC_SLOT m_Looped = MX_INACTIVE;

static FAKE_SLOT m_Slots[FAKE_MUSIC_SLOT_COUNT];

static void M_Reset(void)
{
    m_Playing = MX_INACTIVE;
    m_Looped = MX_INACTIVE;
    for (int32_t i = 0; i < FAKE_MUSIC_SLOT_COUNT; i++) {
        m_Slots[i] = (FAKE_SLOT) {};
    }
}

int32_t Music_PlayBySlot(const MUSIC_SLOT track, const MUSIC_PLAY_MODE mode)
{
    FAKE_RECORD("play", FV(track), FV(mode));
    if (track != FAKE_MUSIC_TRACK) {
        return -1;
    }
    m_Playing = track;
    // Land it on the main stream, or the first overlay for OVERLAY mode, and
    // report the slot.
    const int32_t slot = mode == MPM_OVERLAY ? 1 : 0;
    m_Slots[slot] = (FAKE_SLOT) {
        .active = true,
        .track_id = track,
        .mode = mode,
        .timestamp = 0.0,
    };
    return slot;
}

MUSIC_SLOT Music_GetCurrentPlayingTrack(void)
{
    return m_Playing;
}

MUSIC_SLOT Music_GetCurrentLoopedTrack(void)
{
    return m_Looped;
}

bool Music_IsTrackAvailableBySlot(const MUSIC_SLOT track)
{
    return track == FAKE_MUSIC_TRACK;
}

int32_t Music_GetTrackLimit(void)
{
    return FAKE_MUSIC_TRACK + 1;
}

char *Music_GetTrackPath(const MUSIC_SLOT track)
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
    FAKE_RECORD("stream_stop", FV(slot));
    if (slot >= 0 && slot < FAKE_MUSIC_SLOT_COUNT) {
        m_Slots[slot].active = false;
    }
}

void Music_PauseStream(const int32_t slot)
{
    FAKE_RECORD("stream_pause", FV(slot));
}

void Music_UnpauseStream(const int32_t slot)
{
    FAKE_RECORD("stream_unpause", FV(slot));
}

RESULT Music_SeekStream(const int32_t slot, const double timestamp)
{
    FAKE_RECORD("stream_seek", FV(slot), FV(timestamp));
    FAIL_IF(
        slot < 0 || slot >= FAKE_MUSIC_SLOT_COUNT || !m_Slots[slot].active,
        "slot %d holds no music", slot);
    return OK;
}

void Music_Pause(void)
{
    FAKE_RECORD("pause");
}

void Music_Unpause(void)
{
    FAKE_RECORD("unpause");
}

void Music_Stop(void)
{
    FAKE_RECORD("stop");
    m_Playing = MX_INACTIVE;
}

FAKE_ON_RESET(M_Reset)

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
    m_Playing = (MUSIC_SLOT)track;
}

void FakeMusic_SetLooped(const int32_t track)
{
    m_Looped = (MUSIC_SLOT)track;
}
