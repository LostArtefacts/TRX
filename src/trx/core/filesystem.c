#include <trx/core/filesystem.h>

#include <trx/core/filesystem/priv.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/debug.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
    #include <io.h>
    #include <sys/stat.h>
    #define PATH_SEPARATOR "\\"
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #define PATH_SEPARATOR "/"
#endif

#if defined(_WIN32)
    #include <wchar.h>
    #include <stdlib.h>
    #include <string.h>
    #include <windows.h>

typedef struct {
    _WDIR *handle;
    char *name;
} M_DIR;

static wchar_t *M_UTF8ToWide(const char *const utf8_str)
{
    if (utf8_str == nullptr) {
        return nullptr;
    }
    const size_t len = strlen(utf8_str);
    const size_t wide_len =
        MultiByteToWideChar(CP_UTF8, 0, utf8_str, len, nullptr, 0);
    wchar_t *wide_str = Memory_Alloc((wide_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, len, wide_str, wide_len);
    wide_str[wide_len] = L'\0';
    return wide_str;
}

static char *M_WideToUTF8(const wchar_t *const wide_str)
{
    if (wide_str == nullptr) {
        return nullptr;
    }
    const int32_t len = WideCharToMultiByte(
        CP_UTF8, 0, wide_str, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return nullptr;
    }
    char *const utf8_str = Memory_Alloc(len);
    WideCharToMultiByte(
        CP_UTF8, 0, wide_str, -1, utf8_str, len, nullptr, nullptr);
    return utf8_str;
}

static FILE *M_UTF8Fopen(const char *path, const char *mode)
{
    if (path == nullptr || mode == nullptr) {
        return nullptr;
    }
    wchar_t *const wide_path = M_UTF8ToWide(path);
    wchar_t *const wide_mode = M_UTF8ToWide(mode);
    FILE *const file = _wfopen(wide_path, wide_mode);
    Memory_Free(wide_path);
    Memory_Free(wide_mode);
    return file;
}

#else
static FILE *M_UTF8Fopen(const char *path, const char *mode)
{
    if (path == nullptr || mode == nullptr) {
        return nullptr;
    }
    return fopen(path, mode);
}
#endif

static bool M_ExistsRaw(const char *path)
{
    if (path == nullptr) {
        return false;
    }
    FILE *fp = File_PlatformFopen(path, "rb");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
}

FILE *File_PlatformFopen(const char *const path, const char *const mode)
{
    return M_UTF8Fopen(path, mode);
}

bool File_IsAbsolute(const char *path)
{
    return path && (path[0] == '/' || strstr(path, ":\\"));
}

bool File_IsRelative(const char *path)
{
    return path && !File_IsAbsolute(path);
}

bool File_DirExists(const char *path)
{
    if (path == nullptr) {
        return false;
    }
#if defined(_WIN32)
    wchar_t *const wide_path = M_UTF8ToWide(path);
    const DWORD attrs = GetFileAttributesW(wide_path);
    Memory_Free(wide_path);
    return attrs != INVALID_FILE_ATTRIBUTES
        && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    DIR *dir = opendir(path);
    if (dir != nullptr) {
        closedir(dir);
        return true;
    }
    return false;
#endif
}

bool File_Exists(const char *path)
{
    if (path == nullptr) {
        return false;
    }
    return M_ExistsRaw(path);
}

char *File_GetCurrentDirectory(void)
{
#if defined(_WIN32)
    wchar_t *const wide_cwd = _wgetcwd(nullptr, 0);
    if (wide_cwd == nullptr) {
        return nullptr;
    }
    char *const cwd = M_WideToUTF8(wide_cwd);
    free(wide_cwd);
    return cwd;
#else
    char *const raw_cwd = getcwd(nullptr, 0);
    if (raw_cwd == nullptr) {
        return nullptr;
    }
    char *const cwd = Memory_DupStr(raw_cwd);
    free(raw_cwd);
    return cwd;
#endif
}

char *File_GetParentDirectory(const char *path)
{
    if (path == nullptr) {
        return nullptr;
    }

    const char *const last_delim = MAX(strrchr(path, '/'), strrchr(path, '\\'));
    if (last_delim != nullptr) {
        return String_Format("%.*s", (int32_t)(last_delim - path), path);
    }

    return nullptr;
}

const char *File_GetBaseName(const char *const path)
{
    if (path == nullptr) {
        return nullptr;
    }

    const char *const last_delim = MAX(strrchr(path, '/'), strrchr(path, '\\'));
    return last_delim != nullptr ? last_delim + 1 : path;
}

char *File_GetStem(const char *const path)
{
    if (path == nullptr) {
        return nullptr;
    }

    const char *const name = File_GetBaseName(path);
    const char *const dot = strrchr(name, '.');
    const size_t len = dot != nullptr ? (size_t)(dot - name) : strlen(name);
    return String_Format("%.*s", (int)len, name);
}

bool File_GetMeta(
    const char *const path, uint64_t *const out_size, uint64_t *const out_mtime)
{
#if defined(_WIN32)
    wchar_t *wide_path = M_UTF8ToWide(path);
    if (wide_path == nullptr) {
        return false;
    }
    struct _stat64 st;
    const bool ok = _wstat64(wide_path, &st) == 0;
    Memory_FreePointer(&wide_path);
#else
    struct stat st;
    const bool ok = stat(path, &st) == 0;
#endif
    if (!ok) {
        return false;
    }

    if (out_size != nullptr) {
        *out_size = (uint64_t)st.st_size;
    }
    if (out_mtime != nullptr) {
        *out_mtime = (uint64_t)st.st_mtime;
    }
    return true;
}

bool File_Load(const char *path, char **output_data, size_t *output_size)
{
    ASSERT(output_data != nullptr);

    MYFILE *fp = File_OpenPath(path, FILE_OPEN_READ);
    if (!fp) {
        LOG_ERROR("Can't open file %s", path);
        *output_data = nullptr;
        return false;
    }

    size_t data_size = File_Size(fp);
    char *data = Memory_Alloc(data_size + 1);
    File_ReadData(fp, data, data_size);
    if (File_Pos(fp) != data_size) {
        *output_data = nullptr;
        LOG_ERROR("Can't read file %s", path);
        Memory_FreePointer(&data);
        File_Close(fp);
        return false;
    }
    File_Close(fp);
    data[data_size] = '\0';

    *output_data = data;
    if (output_size != nullptr) {
        *output_size = data_size;
    }
    return true;
}

void File_CreateDirectory(const char *path)
{
    if (path == nullptr) {
        return;
    }
#if defined(_WIN32)
    wchar_t *const wide_path = M_UTF8ToWide(path);
    _wmkdir(wide_path);
    Memory_Free(wide_path);
#else
    mkdir(path, 0775);
#endif
}

bool File_Delete(const char *const path)
{
    if (path == nullptr) {
        return false;
    }
#if defined(_WIN32)
    wchar_t *const wide_path = M_UTF8ToWide(path);
    const bool result = _wremove(wide_path) == 0;
    Memory_Free(wide_path);
    return result;
#else
    return remove(path) == 0;
#endif
}

void File_EnsureParentDirectories(const char *path)
{
    ASSERT(path != nullptr);
    char *parent = File_GetParentDirectory(path);
    if (parent != nullptr) {
        /* Only recurse/create if there is a distinct, non-empty parent */
        if (parent[0] != '\0' && strcmp(parent, path) != 0) {
            if (!File_DirExists(parent)) {
                File_EnsureParentDirectories(parent);
                File_CreateDirectory(parent);
            }
        }
        Memory_FreePointer(&parent);
    }
}

void *File_OpenDirectory(const char *const path)
{
    ASSERT(path != nullptr);
#if defined(_WIN32)
    wchar_t *const wide_path = M_UTF8ToWide(path);
    _WDIR *const handle = _wopendir(wide_path);
    Memory_Free(wide_path);
    if (handle == nullptr) {
        return nullptr;
    }
    M_DIR *const dir = Memory_Alloc(sizeof(M_DIR));
    dir->handle = handle;
    return dir;
#else
    return opendir(path);
#endif
}

const char *File_ReadDirectory(void *const dir)
{
#if defined(_WIN32)
    M_DIR *const win_dir = dir;
    Memory_FreePointer(&win_dir->name);
    const struct _wdirent *const cur_file = _wreaddir(win_dir->handle);
    if (cur_file == nullptr) {
        return nullptr;
    }
    win_dir->name = M_WideToUTF8(cur_file->d_name);
    return win_dir->name;
#else
    const struct dirent *const cur_file = readdir(dir);
    if (cur_file == nullptr) {
        return nullptr;
    }
    return cur_file->d_name;
#endif
}

void File_CloseDirectory(void *const dir)
{
    ASSERT(dir != nullptr);
#if defined(_WIN32)
    M_DIR *const win_dir = dir;
    _wclosedir(win_dir->handle);
    Memory_FreePointer(&win_dir->name);
    Memory_Free(win_dir);
#else
    closedir(dir);
#endif
}
