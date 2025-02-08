#include "game/music/music_backend_cdaudio.h"

#include <libtrx/debug.h>
#include <libtrx/engine/audio.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/music/const.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <global/const.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    uint64_t from;
    uint64_t to;
    bool active;
} CDAUDIO_TRACK;

typedef struct {
    const char *path;
    const char *description;
    CDAUDIO_TRACK *tracks;
} BACKEND_DATA;

static bool M_Parse(BACKEND_DATA *data);
static bool M_Init(MUSIC_BACKEND *backend);
static const char *M_Describe(const MUSIC_BACKEND *backend);
static int32_t M_Play(const MUSIC_BACKEND *backend, int32_t track_id);
static void M_Shutdown(MUSIC_BACKEND *backend);

static bool M_Parse(BACKEND_DATA *const data)
{
    ASSERT(data != nullptr);

    char *track_content = nullptr;
    size_t track_content_size;
    if (!File_Load("audio/cdaudio.dat", &track_content, &track_content_size)) {
        LOG_WARNING("Cannot find CDAudio control file");
        return false;
    }

    data->tracks = Memory_Alloc(sizeof(CDAUDIO_TRACK) * MAX_MUSIC_TRACKS);

    size_t offset = 0;
    while (offset < track_content_size) {
        while (track_content[offset] == '\n' || track_content[offset] == '\r') {
            if (++offset >= track_content_size) {
                goto parse_end;
            }
        }

        uint64_t track_num;
        uint64_t from;
        uint64_t to;
        int32_t result = sscanf(
            &track_content[offset], "%" PRIu64 " %" PRIu64 " %" PRIu64,
            &track_num, &from, &to);

        if (result == 3 && track_num > 0 && track_num <= MAX_MUSIC_TRACKS) {
            int32_t track_idx = track_num - 1;
            data->tracks[track_idx].active = true;
            data->tracks[track_idx].from = from;
            data->tracks[track_idx].to = to;
        }

        while (track_content[offset] != '\n' && track_content[offset] != '\r') {
            if (++offset >= track_content_size) {
                goto parse_end;
            }
        }
    }

parse_end:
    Memory_Free(track_content);

    // reindex wrong track boundaries
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        if (!data->tracks[i].active) {
            continue;
        }

        if (i < MAX_MUSIC_TRACKS - 1
            && data->tracks[i].from >= data->tracks[i].to) {
            for (int32_t j = i + 1; j < MAX_MUSIC_TRACKS; j++) {
                if (data->tracks[j].active) {
                    data->tracks[i].to = data->tracks[j].from;
                    break;
                }
            }
        }

        if (data->tracks[i].from >= data->tracks[i].to && i > 0) {
            for (int32_t j = i - 1; j >= 0; j--) {
                if (data->tracks[j].active) {
                    data->tracks[i].from = data->tracks[j].to;
                    break;
                }
            }
        }
    }

    return true;
}

static bool M_Init(MUSIC_BACKEND *const backend)
{
    ASSERT(backend != nullptr);
    BACKEND_DATA *data = backend->data;
    ASSERT(data != nullptr);

    MYFILE *const fp = File_Open(data->path, FILE_OPEN_READ);
    if (fp == nullptr) {
        return false;
    }

    if (!M_Parse(data)) {
        LOG_ERROR("Failed to parse CDAudio data");
        return false;
    }

    return true;
}

static const char *M_Describe(const MUSIC_BACKEND *const backend)
{
    ASSERT(backend != nullptr);
    const BACKEND_DATA *const data = backend->data;
    ASSERT(data != nullptr);
    return data->description;
}

static int32_t M_Play(
    const MUSIC_BACKEND *const backend, const int32_t track_id)
{
    ASSERT(backend != nullptr);
    const BACKEND_DATA *const data = backend->data;
    ASSERT(data != nullptr);

    const int32_t track_idx = track_id - 1;
    const CDAUDIO_TRACK *track = &data->tracks[track_idx];
    if (track_idx < 0 || track_idx >= MAX_MUSIC_TRACKS) {
        LOG_ERROR("Invalid track: %d", track_id);
        return -1;
    }

    if (!track->active) {
        LOG_ERROR("Invalid track: %d", track_id);
        return -1;
    }

    int32_t audio_stream_id = Audio_Stream_CreateFromFile(data->path);
    Audio_Stream_SetStartTimestamp(audio_stream_id, track->from / 1000.0);
    Audio_Stream_SetStopTimestamp(audio_stream_id, track->to / 1000.0);
    Audio_Stream_SeekTimestamp(audio_stream_id, track->from / 1000.0);
    return audio_stream_id;
}

static void M_Shutdown(MUSIC_BACKEND *backend)
{
    if (backend == nullptr) {
        return;
    }

    if (backend->data != nullptr) {
        BACKEND_DATA *const data = backend->data;
        Memory_FreePointer(&data->path);
        Memory_FreePointer(&data->description);
        Memory_FreePointer(&data->tracks);
    }
    Memory_FreePointer(&backend->data);
    Memory_FreePointer(&backend);
}

MUSIC_BACKEND *Music_Backend_CDAudio_Factory(const char *path)
{
    ASSERT(path != nullptr);

    const char *description_fmt = "CDAudio (path: %s)";
    const size_t description_size = snprintf(nullptr, 0, description_fmt, path);
    char *description = Memory_Alloc(description_size + 1);
    sprintf(description, description_fmt, path);

    BACKEND_DATA *data = Memory_Alloc(sizeof(BACKEND_DATA));
    data->path = Memory_DupStr(path);
    data->description = description;

    MUSIC_BACKEND *backend = Memory_Alloc(sizeof(MUSIC_BACKEND));
    backend->data = data;
    backend->init = M_Init;
    backend->describe = M_Describe;
    backend->play = M_Play;
    backend->shutdown = M_Shutdown;
    return backend;
}
