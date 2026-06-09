#include <trx/game/music/backend_files.h>

#include <trx/av/audio.h>
#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/music/const.h>
#include <trx/game/shell/paths.h>

#include <stdio.h>
#include <string.h>

typedef struct {
    const char *dir;
    const char *description;
    bool available_tracks[MAX_MUSIC_TRACKS];
    int32_t track_limit;
} M_BACKEND_DATA;

static const char *m_ExtensionsToTry[] = {
    ".flac", ".ogg", ".mp3", ".wav", ".wma", nullptr,
};

static char *M_GetTrackFileName(const char *base_dir, int32_t track)
{
    char *tmp_path = String_Format("%s/track%02d.flac", base_dir, track);
    char *result = TRXPath_GuessExtension(tmp_path, m_ExtensionsToTry);
    Memory_FreePointer(&tmp_path);

    if (result == nullptr) {
        tmp_path = String_Format("%s/%d.flac", base_dir, track);
        result = TRXPath_GuessExtension(tmp_path, m_ExtensionsToTry);
        Memory_FreePointer(&tmp_path);
    }
    return result;
}

static bool M_IsSupportedExtension(const char *const ext)
{
    for (int32_t i = 0; m_ExtensionsToTry[i] != nullptr; i++) {
        if (String_Equivalent(ext, m_ExtensionsToTry[i])) {
            return true;
        }
    }
    return false;
}

static bool M_ParseTrackID(
    const char *const entry_name, const char *const prefix, int32_t *const out)
{
    const size_t prefix_len = strlen(prefix);
    if (prefix_len > 0
        && String_CaseSubstring(entry_name, prefix) != entry_name) {
        return false;
    }

    int32_t track_id = -1;
    int32_t name_len = 0;
    if (sscanf(entry_name + prefix_len, "%d%n", &track_id, &name_len) != 1) {
        return false;
    }

    const char *const ext = entry_name + prefix_len + name_len;
    if (ext[0] != '.' || !M_IsSupportedExtension(ext)) {
        return false;
    }

    const char *const canonical_entry = prefix_len > 0
        ? String_FormatStatic("track%02d%s", track_id, ext)
        : String_FormatStatic("%d%s", track_id, ext);
    if (String_Equivalent(entry_name, canonical_entry)) {
        *out = track_id;
        return true;
    }

    return false;
}

static bool M_TryParseTrackID(const char *const entry_name, int32_t *const out)
{
    return M_ParseTrackID(entry_name, "track", out)
        || M_ParseTrackID(entry_name, "", out);
}

static void M_MarkTrackAvailable(
    M_BACKEND_DATA *const data, const int32_t track_id)
{
    ASSERT(data != nullptr);

    if (track_id < 0 || track_id >= MAX_MUSIC_TRACKS) {
        return;
    }

    data->available_tracks[track_id] = true;
    if (track_id + 1 > data->track_limit) {
        data->track_limit = track_id + 1;
    }
}

static void M_IndexAvailableTracks(M_BACKEND_DATA *const data)
{
    ASSERT(data != nullptr);
    void *const dir = File_OpenDirectory(data->dir);
    if (dir == nullptr) {
        return;
    }

    const char *entry_name = nullptr;
    while ((entry_name = File_ReadDirectory(dir)) != nullptr) {
        int32_t track_id = -1;
        if (!M_TryParseTrackID(entry_name, &track_id)) {
            continue;
        }
        M_MarkTrackAvailable(data, track_id);
    }

    File_CloseDirectory(dir);
}

static bool M_IsTrackAvailableValue(const int32_t value, void *const user_data)
{
    const M_BACKEND_DATA *const data = user_data;
    ASSERT(data != nullptr);

    return value >= 0 && value < MAX_MUSIC_TRACKS
        && data->available_tracks[value];
}

static int32_t M_CountAvailableTracks(const M_BACKEND_DATA *const data)
{
    ASSERT(data != nullptr);

    int32_t result = 0;
    for (int32_t i = 0; i < data->track_limit; i++) {
        if (data->available_tracks[i]) {
            result++;
        }
    }
    return result;
}

static bool M_Init(MUSIC_BACKEND *const backend)
{
    ASSERT(backend != nullptr);
    M_BACKEND_DATA *const data = backend->data;
    ASSERT(data->dir != nullptr);
    if (!File_DirExists(data->dir)) {
        return false;
    }

    data->track_limit = 0;
    M_IndexAvailableTracks(data);

    const int32_t track_count = M_CountAvailableTracks(data);
    if (data->track_limit > 0) {
        char *ranges = String_FormatRanges(
            0, data->track_limit - 1, M_IsTrackAvailableValue, data);
        LOG_INFO(
            "Indexed %d music track(s) from %s: %s", track_count, data->dir,
            ranges);
        Memory_FreePointer(&ranges);
    } else {
        LOG_INFO("Indexed 0 music tracks from %s", data->dir);
    }
    return true;
}

static const char *M_Describe(const MUSIC_BACKEND *const backend)
{
    ASSERT(backend != nullptr);
    const M_BACKEND_DATA *const data = backend->data;
    ASSERT(data != nullptr);
    return data->description;
}

static bool M_IsTrackAvailable(
    const MUSIC_BACKEND *const backend, const int32_t track_id)
{
    ASSERT(backend != nullptr);
    const M_BACKEND_DATA *const data = backend->data;
    ASSERT(data != nullptr);

    return track_id >= 0 && track_id < MAX_MUSIC_TRACKS
        && data->available_tracks[track_id];
}

static int32_t M_GetTrackLimit(const MUSIC_BACKEND *const backend)
{
    ASSERT(backend != nullptr);
    const M_BACKEND_DATA *const data = backend->data;
    ASSERT(data != nullptr);
    return data->track_limit;
}

static int32_t M_Play(
    const MUSIC_BACKEND *const backend, const int32_t track_id)
{
    ASSERT(backend != nullptr);
    const M_BACKEND_DATA *const data = backend->data;
    ASSERT(data != nullptr);

    char *file_path = M_GetTrackFileName(data->dir, track_id);
    if (file_path == nullptr) {
        LOG_ERROR("Invalid track: %d", track_id);
        return -1;
    }

    const int32_t stream_id = Audio_Stream_CreateFromFile(file_path);
    Memory_Free(file_path);
    return stream_id;
}

static void M_Shutdown(MUSIC_BACKEND *backend)
{
    if (backend == nullptr) {
        return;
    }

    if (backend->data != nullptr) {
        M_BACKEND_DATA *const data = backend->data;
        Memory_FreePointer(&data->dir);
        Memory_FreePointer(&data->description);
    }
    Memory_FreePointer(&backend->data);
    Memory_FreePointer(&backend);
}

MUSIC_BACKEND *Music_Backend_Files_Factory(const char *path)
{
    ASSERT(path != nullptr);

    const char *description_fmt = "Directory (directory: %s)";
    const size_t description_size = snprintf(nullptr, 0, description_fmt, path);
    char *description = Memory_Alloc(description_size + 1);
    sprintf(description, description_fmt, path);

    M_BACKEND_DATA *const data = Memory_Alloc(sizeof(M_BACKEND_DATA));
    data->dir = Memory_DupStr(path);
    data->description = description;

    MUSIC_BACKEND *const backend = Memory_Alloc(sizeof(MUSIC_BACKEND));
    backend->data = data;
    backend->init = M_Init;
    backend->describe = M_Describe;
    backend->is_track_available = M_IsTrackAvailable;
    backend->get_track_limit = M_GetTrackLimit;
    backend->play = M_Play;
    backend->shutdown = M_Shutdown;
    return backend;
}
