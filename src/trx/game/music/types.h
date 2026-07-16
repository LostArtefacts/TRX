#pragma once

#include <stdint.h>

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
