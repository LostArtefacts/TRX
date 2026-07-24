#pragma once

#include <trx/game/music/enum.h>

#include <stdint.h>

// A track's accumulated trigger state. The mask holds the floordata code bits
// (MTF_CODE_BITS), as the packed save word does; `delay` is the TR2
// delayed-play countdown in frames.
typedef struct {
    uint16_t mask;
    bool is_one_shot;
    uint8_t delay;
} MUSIC_TRACK_STATE;

// The lean description a music trigger acts on, mapped from floordata at
// the trigger-handler boundary. `timer` is in seconds.
typedef struct {
    MUSIC_TRIGGER_KIND kind;
    uint16_t mask;
    int8_t timer;
    bool one_shot;
} MUSIC_TRIGGER;

typedef struct MUSIC_BACKEND {
    bool (*init)(struct MUSIC_BACKEND *backend);
    const char *(*describe)(const struct MUSIC_BACKEND *backend);
    bool (*is_track_available)(
        const struct MUSIC_BACKEND *backend, int32_t track_id);
    int32_t (*get_track_limit)(const struct MUSIC_BACKEND *backend);
    // Resolves a track to its file path, freshly allocated for the caller to
    // free, or nullptr when the backend has no file for it. Optional.
    char *(*get_track_path)(
        const struct MUSIC_BACKEND *backend, int32_t track_id);
    int32_t (*play)(const struct MUSIC_BACKEND *backend, int32_t track_id);
    void (*shutdown)(struct MUSIC_BACKEND *backend);
    void *data;
} MUSIC_BACKEND;
