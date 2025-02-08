#include "game/music.h"

static uint16_t m_MusicTrackFlags[MAX_MUSIC_TRACKS] = {};

void Music_ResetTrackFlags(void)
{
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        m_MusicTrackFlags[i] = 0;
    }
}

uint16_t Music_GetTrackFlags(const int32_t track_idx)
{
    return m_MusicTrackFlags[track_idx];
}

void Music_SetTrackFlags(const int32_t track, const uint16_t flags)
{
    m_MusicTrackFlags[track] = flags;
}
