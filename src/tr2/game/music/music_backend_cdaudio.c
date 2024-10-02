#include "game/music/music_backend_cdaudio.h"

#include <libtrx/engine/audio.h>
#include <libtrx/filesystem.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_CD_TRACKS 60

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

static bool M_Parse(BACKEND_DATA *const data)
{
    assert(data != NULL);

    char *track_content = NULL;
    size_t track_content_size;
    if (!File_Load("audio/cdaudio.dat", &track_content, &track_content_size)) {
        LOG_WARNING("Cannot find CDAudio control file");
        return false;
    }

    data->tracks = Memory_Alloc(sizeof(CDAUDIO_TRACK) * MAX_CD_TRACKS);

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

        if (result == 3 && track_num > 0 && track_num <= MAX_CD_TRACKS) {
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
    for (int32_t i = 0; i < MAX_CD_TRACKS; i++) {
        if (!data->tracks[i].active) {
            continue;
        }

        if (i < MAX_CD_TRACKS - 1
            && data->tracks[i].from >= data->tracks[i].to) {
            for (int32_t j = i + 1; j < MAX_CD_TRACKS; j++) {
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
    assert(backend != NULL);
    BACKEND_DATA *data = backend->data;
    assert(data != NULL);

    MYFILE *const fp = File_Open(data->path, FILE_OPEN_READ);
    if (fp == NULL) {
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
    assert(backend != NULL);
    const BACKEND_DATA *const data = backend->data;
    assert(data != NULL);
    return data->description;
}

static int32_t M_Play(
    const MUSIC_BACKEND *const backend, const int32_t track_id)
{
    assert(backend != NULL);
    const BACKEND_DATA *const data = backend->data;
    assert(data != NULL);

    const int32_t track_idx = track_id - 1;
    const CDAUDIO_TRACK *track = &data->tracks[track_idx];
    if (track_idx < 0 || track_idx >= MAX_CD_TRACKS) {
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

MUSIC_BACKEND *Music_Backend_CDAudio_Factory(const char *path)
{
    assert(path != NULL);

    const char *description_fmt = "CDAudio (path: %s)";
    const size_t description_size = snprintf(NULL, 0, description_fmt, path);
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
    return backend;
}

void Music_Backend_CDAudio_Destroy(MUSIC_BACKEND *backend)
{
    if (backend == NULL) {
        return;
    }

    if (backend->data != NULL) {
        BACKEND_DATA *const data = backend->data;
        Memory_FreePointer(&data->path);
        Memory_FreePointer(&data->description);
        Memory_FreePointer(&data->tracks);
    }
    Memory_FreePointer(&backend->data);
    Memory_FreePointer(&backend);
}
