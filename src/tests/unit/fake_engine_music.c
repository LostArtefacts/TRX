// A soundtrack with exactly one track in it. Playing anything else fails, which
// is how the engine reports a track that is not there.

#include "fake_engine_music.h"

#include <trx/game/music/ids.h>

#include <stdbool.h>

FAKE_MUSIC_CALLS g_FakeMusicCalls;
static MUSIC_ID m_Playing = MX_INACTIVE;

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
}

void FakeMusic_SetPlaying(const int32_t track)
{
    m_Playing = (MUSIC_ID)track;
}
