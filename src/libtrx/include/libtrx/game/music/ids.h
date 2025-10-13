#pragma once

#include <stdint.h>

typedef enum {
    MX_INACTIVE = -1,
#define X_MUSIC_TRACK_DEFINE(name, game1_id, game2_id) name,
#include "./ids.def"
#undef X_MUSIC_TRACK_DEFINE
    MX_NUMBER_OF,
} MUSIC_TRACK;

int32_t Music_GetTrackID(MUSIC_TRACK music_track);
MUSIC_TRACK Music_UnmapTrackID(int32_t track_id);
