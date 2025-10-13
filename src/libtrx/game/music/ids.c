#include "game/catalog.h"
#include "game/music.h"

int32_t Music_GetTrackID(const MUSIC_TRACK music_track)
{
    int32_t out;
    if (Catalog_EnumToGameID(CATALOG_MUSIC, music_track, &out)) {
        return out;
    }
    return -1;
}

MUSIC_TRACK Music_UnmapTrackID(const int32_t track_id)
{
    CATALOG_ID out;
    if (Catalog_GameIDToEnum(CATALOG_MUSIC, track_id, &out)) {
        return out;
    }
    return MX_INACTIVE;
}
