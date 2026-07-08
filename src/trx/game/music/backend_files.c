#include <trx/game/music/backend_files.h>

#include <trx/av/audio.h>
#include <trx/core/csv.h>
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
    const char *catalog_path;
    const char *description;
    char *catalog_tracks[MAX_MUSIC_TRACKS];
    bool available_tracks[MAX_MUSIC_TRACKS];
    int32_t track_limit;
} M_BACKEND_DATA;

static const char *m_ExtensionsToTry[] = {
    ".flac", ".ogg", ".mp3", ".wav", ".wma", nullptr,
};

static char *M_GetTrackFileName(
    const M_BACKEND_DATA *const data, const int32_t track)
{
    ASSERT(data != nullptr);

    if (track >= 0 && track < MAX_MUSIC_TRACKS
        && data->catalog_tracks[track] != nullptr) {
        return Memory_DupStr(data->catalog_tracks[track]);
    }

    if (data->dir == nullptr) {
        return nullptr;
    }

    char *tmp_path = String_Format("%s/track%02d.flac", data->dir, track);
    char *result = TRXPath_GuessExtension(tmp_path, m_ExtensionsToTry);
    Memory_FreePointer(&tmp_path);

    if (result == nullptr) {
        tmp_path = String_Format("%s/%d.flac", data->dir, track);
        result = TRXPath_GuessExtension(tmp_path, m_ExtensionsToTry);
        Memory_FreePointer(&tmp_path);
    }
    return result;
}

static bool M_ParseCatalogTrackID(
    const char *const value, int32_t *const out_track_id)
{
    ASSERT(value != nullptr);
    ASSERT(out_track_id != nullptr);

    int32_t track_id = -1;
    int32_t parsed_len = 0;
    if (sscanf(value, "%d%n", &track_id, &parsed_len) != 1
        || value[parsed_len] != '\0') {
        return false;
    }

    *out_track_id = track_id;
    return true;
}

static char *M_GetCatalogFilePath(
    const char *const catalog_dir, const char *const file_path)
{
    ASSERT(catalog_dir != nullptr);
    ASSERT(file_path != nullptr);

    if (File_IsAbsolute(file_path)) {
        return TRXPath_GuessExtension(file_path, m_ExtensionsToTry);
    }

    const char *resolved_path =
        TRXPath_PeekResolve(TRX_DYNAMIC_PATH_CATALOG, file_path);
    if (resolved_path != nullptr) {
        return Memory_DupStr(resolved_path);
    }

    resolved_path =
        TRXPath_PeekResolve(TRX_DYNAMIC_PATH_CDAUDIO_FILE, file_path);
    if (resolved_path != nullptr) {
        return Memory_DupStr(resolved_path);
    }

    char *local_path = String_Format("%s/%s", catalog_dir, file_path);
    char *canonical_path =
        TRXPath_GuessExtension(local_path, m_ExtensionsToTry);
    Memory_FreePointer(&local_path);
    return canonical_path;
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

static void M_LoadCatalogLine(
    M_BACKEND_DATA *const data, const char *const catalog_dir, char *const line,
    const int32_t line_num)
{
    ASSERT(data != nullptr);
    ASSERT(catalog_dir != nullptr);
    ASSERT(line != nullptr);

    char *const trimmed_line = CSV_Trim(line);
    if (trimmed_line[0] == '\0' || trimmed_line[0] == '#') {
        return;
    }

    const char *p = trimmed_line;
    char id_buf[32];
    char path_buf[512];
    CSV_ParseField(&p, id_buf, sizeof(id_buf));
    CSV_ParseField(&p, path_buf, sizeof(path_buf));

    char *const id_str = CSV_Trim(id_buf);
    char *const path_str = CSV_Trim(path_buf);

    p = CSV_SkipWhitespace(p);
    if (*p != '\0') {
        LOG_WARNING(
            "Invalid music file catalog row %s:%d", data->catalog_path,
            line_num);
        return;
    }

    int32_t track_id = -1;
    if (!M_ParseCatalogTrackID(id_str, &track_id) || path_str[0] == '\0') {
        LOG_WARNING(
            "Invalid music file catalog row %s:%d", data->catalog_path,
            line_num);
        return;
    }

    if (track_id < 0 || track_id >= MAX_MUSIC_TRACKS) {
        LOG_WARNING(
            "Music file catalog row %s:%d has out-of-range track ID %d",
            data->catalog_path, line_num, track_id);
        return;
    }

    if (data->catalog_tracks[track_id] != nullptr) {
        LOG_WARNING(
            "Music file catalog row %s:%d duplicates track ID %d; keeping %s",
            data->catalog_path, line_num, track_id,
            data->catalog_tracks[track_id]);
        return;
    }

    char *resolved_path = M_GetCatalogFilePath(catalog_dir, path_str);
    if (resolved_path == nullptr) {
        LOG_WARNING(
            "Music file catalog row %s:%d points to missing file: %s",
            data->catalog_path, line_num, path_str);
        Memory_FreePointer(&resolved_path);
        return;
    }

    data->catalog_tracks[track_id] = resolved_path;
    M_MarkTrackAvailable(data, track_id);
}

static void M_LoadCatalog(M_BACKEND_DATA *const data)
{
    ASSERT(data != nullptr);

    if (data->catalog_path == nullptr) {
        return;
    }

    char *file_data = nullptr;
    size_t file_size = 0;
    if (!File_Load(data->catalog_path, &file_data, &file_size)) {
        return;
    }

    char *catalog_dir = File_GetParentDirectory(data->catalog_path);
    if (catalog_dir == nullptr) {
        LOG_WARNING(
            "Cannot determine parent directory for music file catalog: %s",
            data->catalog_path);
        Memory_FreePointer(&file_data);
        return;
    }

    const char *pos = file_data;
    const char *const end = file_data + file_size;
    char line[1024];
    int32_t line_num = 1;
    while (pos < end) {
        size_t len = 0;
        while (pos < end && *pos != '\n' && len + 1 < sizeof(line)) {
            line[len] = *pos;
            len++;
            pos++;
        }
        while (pos < end && *pos != '\n') {
            pos++;
        }
        if (pos < end && *pos == '\n') {
            pos++;
        }
        if (len > 0 && line[len - 1] == '\r') {
            len--;
        }
        line[len] = '\0';
        M_LoadCatalogLine(data, catalog_dir, line, line_num);
        line_num++;
    }

    Memory_FreePointer(&catalog_dir);
    Memory_FreePointer(&file_data);
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

    data->track_limit = 0;
    M_LoadCatalog(data);

    const bool dir_exists = data->dir != nullptr && File_DirExists(data->dir);
    if (dir_exists) {
        M_IndexAvailableTracks(data);
    } else if (data->track_limit == 0) {
        return false;
    }

    const int32_t track_count = M_CountAvailableTracks(data);
    if (data->track_limit > 0) {
        char *ranges = String_FormatRanges(
            0, data->track_limit - 1, M_IsTrackAvailableValue, data);
        LOG_INFO(
            "Indexed %d music track(s) from %s: %s", track_count,
            data->dir != nullptr ? data->dir : data->catalog_path, ranges);
        Memory_FreePointer(&ranges);
    } else {
        LOG_INFO(
            "Indexed 0 music tracks from %s",
            data->dir != nullptr ? data->dir : data->catalog_path);
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

    char *file_path = M_GetTrackFileName(data, track_id);
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
        for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
            Memory_FreePointer(&data->catalog_tracks[i]);
        }
        Memory_FreePointer(&data->dir);
        Memory_FreePointer(&data->catalog_path);
        Memory_FreePointer(&data->description);
    }
    Memory_FreePointer(&backend->data);
    Memory_FreePointer(&backend);
}

MUSIC_BACKEND *Music_Backend_Files_Factory(
    const char *const path, const char *const catalog_path)
{
    ASSERT(path != nullptr || catalog_path != nullptr);

    const char *description_fmt = "Directory (directory: %s, catalog: %s)";
    const size_t description_size = snprintf(
        nullptr, 0, description_fmt, path != nullptr ? path : "(none)",
        catalog_path != nullptr ? catalog_path : "(none)");
    char *description = Memory_Alloc(description_size + 1);
    sprintf(
        description, description_fmt, path != nullptr ? path : "(none)",
        catalog_path != nullptr ? catalog_path : "(none)");

    M_BACKEND_DATA *const data = Memory_Alloc(sizeof(M_BACKEND_DATA));
    data->dir = path != nullptr ? Memory_DupStr(path) : nullptr;
    data->catalog_path =
        catalog_path != nullptr ? Memory_DupStr(catalog_path) : nullptr;
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
