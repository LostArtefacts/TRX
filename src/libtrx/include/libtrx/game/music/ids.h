#pragma once

#include <stdint.h>

typedef enum {
    MX_INACTIVE = -1,
#define X_CATALOG_ID(uuid_str, enum_value) enum_value,
#include "../catalog_music.def"
#undef X_CATALOG_ID
    MX_NUMBER_OF,
} MUSIC_TRACK;

int32_t Music_GetTrackID(MUSIC_TRACK music_track);
MUSIC_TRACK Music_UnmapTrackID(int32_t track_id);
