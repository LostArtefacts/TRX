#pragma once

typedef enum {
    MPM_ONCE,
    MPM_LOOP,
    MPM_DELAY,
    MPM_NO_REPEAT,
    MPM_OVERLAY,
} MUSIC_PLAY_MODE;

// The flag operation a music trigger performs, decoupled from the floordata
// TRIGGER_TYPE, as with ITEM_TRIGGER_KIND.
typedef enum {
    MUSIC_TRIGGER_NORMAL,
    MUSIC_TRIGGER_SWITCH,
    MUSIC_TRIGGER_ANTI,
} MUSIC_TRIGGER_KIND;
