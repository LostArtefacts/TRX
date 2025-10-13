#include "game/music/ids.h"

#include "utils.h"

static int32_t m_GameIDMap[TR_VERSION_COUNT][MX_NUMBER_OF] = {
    [0] = {
#define X_MUSIC_TRACK_DEFINE(name, g1, g2) [name] = (g1),
#include "game/music/ids.def"
#undef X_MUSIC_TRACK_DEFINE
    },
    [1] = {
#define X_MUSIC_TRACK_DEFINE(name, g1, g2) [name] = (g2),
#include "game/music/ids.def"
#undef X_MUSIC_TRACK_DEFINE
    }
};

int32_t Music_GetTrackID(const MUSIC_TRACK music_track)
{
    if (music_track < 0 || music_track >= MX_NUMBER_OF) {
        return -1;
    }
    return m_GameIDMap[TR_VERSION - 1][music_track];
}

MUSIC_TRACK Music_UnmapTrackID(const int32_t track_id)
{
    for (MUSIC_TRACK track = 0; track < MX_NUMBER_OF; track++) {
        if (m_GameIDMap[TR_VERSION - 1][track] == track_id) {
            return track;
        }
    }
    return MX_INACTIVE;
}
