
#include "game/music/music_backend_files.h"

#include <libtrx/engine/audio.h>
#include <libtrx/filesystem.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <assert.h>

typedef struct {
    const char *dir;
    const char *description;
} BACKEND_DATA;

static const char *m_ExtensionsToTry[] = { ".flac", ".ogg", ".mp3", ".wav",
                                           NULL };

static char *M_GetTrackFileName(const char *base_dir, int32_t track);
static const char *M_Describe(const MUSIC_BACKEND *backend);
static bool M_Init(MUSIC_BACKEND *backend);
static int32_t M_Play(const MUSIC_BACKEND *backend, int32_t track_id);

static char *M_GetTrackFileName(const char *base_dir, int32_t track)
{
    char file_path[64];
    sprintf(file_path, "%s/track%02d.flac", base_dir, track);
    char *result = File_GuessExtension(file_path, m_ExtensionsToTry);
    if (!File_Exists(file_path)) {
        Memory_FreePointer(&result);
        sprintf(file_path, "%s/%d.flac", base_dir, track);
        result = File_GuessExtension(file_path, m_ExtensionsToTry);
    }
    return result;
}

static bool M_Init(MUSIC_BACKEND *const backend)
{
    assert(backend != NULL);
    const BACKEND_DATA *data = backend->data;
    assert(data->dir != NULL);
    return File_DirExists(data->dir);
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

    char *file_path = M_GetTrackFileName(data->dir, track_id);
    if (file_path == NULL) {
        LOG_ERROR("Invalid track: %d", track_id);
        return -1;
    }

    return Audio_Stream_CreateFromFile(file_path);
}

MUSIC_BACKEND *Music_Backend_Files_Factory(const char *path)
{
    assert(path != NULL);

    const char *description_fmt = "Directory (directory: %s)";
    const size_t description_size = snprintf(NULL, 0, description_fmt, path);
    char *description = Memory_Alloc(description_size + 1);
    sprintf(description, description_fmt, path);

    BACKEND_DATA *data = Memory_Alloc(sizeof(BACKEND_DATA));
    data->dir = Memory_DupStr(path);
    data->description = description;

    MUSIC_BACKEND *backend = Memory_Alloc(sizeof(MUSIC_BACKEND));
    backend->data = data;
    backend->init = M_Init;
    backend->describe = M_Describe;
    backend->play = M_Play;
    return backend;
}

void Music_Backend_Files_Destroy(MUSIC_BACKEND *backend)
{
    if (backend == NULL) {
        return;
    }

    if (backend->data != NULL) {
        BACKEND_DATA *const data = backend->data;
        Memory_FreePointer(&data->dir);
        Memory_FreePointer(&data->description);
    }
    Memory_FreePointer(&backend->data);
    Memory_FreePointer(&backend);
}
