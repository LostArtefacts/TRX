#pragma once

typedef enum {
    MPM_ONCE,
    MPM_LOOP,
    MPM_DELAY,
    MPM_NO_REPEAT,
    MPM_OVERLAY,
} MUSIC_PLAY_MODE;

// Flags of a music track's packed trigger word in save files; the low byte
// carries the TR2 delay counter instead. The values are fixed by the
// floordata trigger encoding.
typedef enum {
    // clang-format off
    MTF_ONE_SHOT  = 0x0100,
    MTF_CODE_BITS = 0x3E00,
    // clang-format on
} MUSIC_TRACK_FLAG;

// The flag operation a music trigger performs, decoupled from the floordata
// TRIGGER_TYPE.
typedef enum {
    MUSIC_TRIGGER_NORMAL,
    MUSIC_TRIGGER_SWITCH,
    MUSIC_TRIGGER_ANTI,
} MUSIC_TRIGGER_KIND;
