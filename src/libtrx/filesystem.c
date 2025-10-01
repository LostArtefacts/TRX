#include "filesystem.h"

#include "debug.h"
#include "log.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <SDL2/SDL_filesystem.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
    #define PATH_SEPARATOR "\\"
#else
    #include <sys/stat.h>
    #define PATH_SEPARATOR "/"
#endif

struct MYFILE {
    FILE *fp;
    const char *path;
};

const char *m_GameDir = nullptr;

#if defined(_WIN32)
    #include <wchar.h>
    #include <stdlib.h>
    #include <string.h>
    #include <windows.h>

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
    return fopen(path, mode);
}
#endif

static void M_PathAppendSeparator(char *const path)
{
    if (!String_EndsWith(path, PATH_SEPARATOR)) {
        strcat(path, PATH_SEPARATOR);
    }
}

static void M_PathAppendPart(char *const path, const char *const part)
{
    M_PathAppendSeparator(path);
    strcat(path, part);
}

static bool M_ExistsRaw(const char *path)
{
    FILE *fp = M_UTF8Fopen(path, "rb");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
}

static char *M_CasePath(const char *const path)
{
    ASSERT(path != nullptr);

    char *path_copy = Memory_DupStr(path);
    if (M_ExistsRaw(path)) {
        return path_copy;
    }

    char *path_piece = path_copy;
    char *current_path = Memory_Alloc(strlen(path) + 2);

    if (path_copy[0] == '/') {
        strcpy(current_path, "/");
        path_piece++;
    } else if (strstr(path_copy, ":\\")) {
        strcpy(current_path, path_copy);
        strstr(current_path, ":\\")[1] = '\0';
        path_piece += 3;
    } else {
        strcpy(current_path, ".");
    }

    while (path_piece) {
        char *delim = strpbrk(path_piece, "/\\");
        char old_delim = delim ? *delim : '\0';
        if (delim) {
            *delim = '\0';
        }

        DIR *path_dir = opendir(current_path);
        if (!path_dir) {
            Memory_FreePointer(&path_copy);
            Memory_FreePointer(&current_path);
            return nullptr;
        }

        struct dirent *cur_file = readdir(path_dir);
        while (cur_file) {
            if (String_Equivalent(path_piece, cur_file->d_name)) {
                M_PathAppendPart(current_path, cur_file->d_name);
                break;
            }
            cur_file = readdir(path_dir);
        }
        closedir(path_dir);

        if (!cur_file) {
            M_PathAppendPart(current_path, path_piece);
        }

        if (delim) {
            *delim = old_delim;
            path_piece = delim + 1;
        } else {
            break;
        }
    }

    Memory_FreePointer(&path_copy);

    char *result;
    if (current_path[0] == '.'
        && strcmp(current_path + 1, PATH_SEPARATOR)
            == 0) { /* strip leading ./ */
        result = Memory_DupStr(current_path + 1 + strlen(PATH_SEPARATOR));
    } else {
        result = Memory_DupStr(current_path);
    }
    Memory_FreePointer(&current_path);
    return result;
}

static FILE *M_ResolveAndOpen(
    const char *const path, const char *const mode, char **out_full_path)
{
    char *abs_path = nullptr;
    if (File_IsRelative(path)) {
        const char *game_dir = File_GetGameDirectory();
        if (game_dir != nullptr) {
            abs_path = String_Format("%s%s", game_dir, path);
        }
    }
    if (abs_path == nullptr) {
        abs_path = Memory_DupStr(path);
    }

    FILE *fp = M_UTF8Fopen(abs_path, mode);
    char *resolved_path = nullptr;
    if (fp != nullptr) {
        resolved_path = Memory_DupStr(abs_path);
        goto finish;
    } else {
        resolved_path = M_CasePath(abs_path);
        if (resolved_path != nullptr) {
            fp = M_UTF8Fopen(resolved_path, mode);
        }
    }

finish:
    if (out_full_path != nullptr) {
        *out_full_path = resolved_path;
    } else {
        Memory_FreePointer(&resolved_path);
    }
    Memory_FreePointer(&abs_path);
    return fp;
}

bool File_IsAbsolute(const char *path)
{
    return path && (path[0] == '/' || strstr(path, ":\\"));
}

bool File_IsRelative(const char *path)
{
    return path && !File_IsAbsolute(path);
}

const char *File_GetGameDirectory(void)
{
    if (m_GameDir == nullptr) {
        m_GameDir = SDL_GetBasePath();
        if (!m_GameDir) {
            LOG_ERROR("Can't get module handle");
            return nullptr;
        }
    }
    return m_GameDir;
}

bool File_DirExists(const char *path)
{
    char *full_path = File_GetFullPath(path);
    DIR *dir = opendir(full_path);
    Memory_FreePointer(&full_path);
    if (dir != nullptr) {
        closedir(dir);
        return true;
    }
    return false;
}

bool File_Exists(const char *path)
{
    char *full_path = File_GetFullPath(path);
    bool ret = M_ExistsRaw(full_path);
    Memory_FreePointer(&full_path);
    return ret;
}

char *File_GetFullPath(const char *path)
{
    char *full_path = nullptr;
    FILE *fp = M_ResolveAndOpen(path, "rb", &full_path);
    if (fp != nullptr) {
        fclose(fp);
    }
    return full_path;
}

char *File_GetParentDirectory(const char *path)
{
    if (path == nullptr) {
        return nullptr;
    }

    char *last_delim = MAX(strrchr(path, '/'), strrchr(path, '\\'));
    if (last_delim != nullptr) {
        return String_Format("%.*s", last_delim - path, path);
    }

    char *full_path = File_GetFullPath(path);
    if (full_path == nullptr) {
        return nullptr;
    }
    last_delim = MAX(strrchr(full_path, '/'), strrchr(full_path, '\\'));
    if (last_delim != nullptr) {
        *last_delim = '\0';
    }
    return full_path;
}

char *File_GuessExtension(const char *path, const char **extensions)
{
    if (File_Exists(path)) {
        goto fallback;
    }

    const char *dot = strrchr(path, '.');
    if (dot == nullptr) {
        goto fallback;
    }

    for (const char **ext = &extensions[0]; *ext; ext++) {
        size_t out_size = dot - path + strlen(*ext) + 1;
        char *out = Memory_Alloc(out_size);
        strncpy(out, path, dot - path);
        out[dot - path] = '\0';
        strcat(out, *ext);

        char *full_path = File_GetFullPath(out);
        Memory_FreePointer(&out);
        if (M_ExistsRaw(full_path)) {
            return full_path;
        }
        Memory_FreePointer(&full_path);
    }

fallback:
    return Memory_DupStr(path);
}

MYFILE *File_Open(const char *path, FILE_OPEN_MODE mode)
{
    MYFILE *file = Memory_Alloc(sizeof(MYFILE));
    file->path = Memory_DupStr(path);
    switch (mode) {
    case FILE_OPEN_WRITE:
        file->fp = M_ResolveAndOpen(path, "wb", nullptr);
        break;
    case FILE_OPEN_READ:
        file->fp = M_ResolveAndOpen(path, "rb", nullptr);
        break;
    case FILE_OPEN_READ_WRITE:
        file->fp = M_ResolveAndOpen(path, "r+b", nullptr);
        break;
    default:
        file->fp = nullptr;
        break;
    }
    if (!file->fp) {
        Memory_FreePointer(&file->path);
        Memory_FreePointer(&file);
    }
    return file;
}

bool File_ReadData(MYFILE *const file, void *const data, const size_t size)
{
    return fread(data, size, 1, file->fp) == 1;
}

bool File_ReadItems(
    MYFILE *const file, void *data, const size_t count, const size_t item_size)
{
    return fread(data, item_size, count, file->fp) == count;
}

int8_t File_ReadS8(MYFILE *const file)
{
    int8_t result;
    File_ReadData(file, &result, sizeof(result));
    return result;
}

int16_t File_ReadS16(MYFILE *const file)
{
    int16_t result;
    File_ReadData(file, &result, sizeof(result));
    return result;
}

int32_t File_ReadS32(MYFILE *const file)
{
    int32_t result;
    File_ReadData(file, &result, sizeof(result));
    return result;
}

uint8_t File_ReadU8(MYFILE *const file)
{
    uint8_t result;
    File_ReadData(file, &result, sizeof(result));
    return result;
}

uint16_t File_ReadU16(MYFILE *const file)
{
    uint16_t result;
    File_ReadData(file, &result, sizeof(result));
    return result;
}

uint32_t File_ReadU32(MYFILE *const file)
{
    uint32_t result;
    File_ReadData(file, &result, sizeof(result));
    return result;
}

void File_WriteData(
    MYFILE *const file, const void *const data, const size_t size)
{
    fwrite(data, size, 1, file->fp);
}

void File_WriteItems(
    MYFILE *const file, const void *const data, const size_t count,
    const size_t item_size)
{
    fwrite(data, item_size, count, file->fp);
}

void File_WriteS8(MYFILE *const file, const int8_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteS16(MYFILE *const file, const int16_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteS32(MYFILE *const file, const int32_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteU8(MYFILE *const file, const uint8_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteU16(MYFILE *const file, const uint16_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteU32(MYFILE *const file, const uint32_t value)
{
    fwrite(&value, sizeof(value), 1, file->fp);
}

void File_WriteString(MYFILE *file, const char *fmt, ...)
{
    if (file == nullptr || file->fp == nullptr) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    const char *s = String_FormatStaticV(fmt, args);
    va_end(args);
    fputs(s, file->fp);
}

void File_Skip(MYFILE *file, size_t bytes)
{
    File_Seek(file, bytes, FILE_SEEK_CUR);
}

void File_Seek(MYFILE *file, size_t pos, FILE_SEEK_MODE mode)
{
    switch (mode) {
    case FILE_SEEK_SET:
        fseek(file->fp, pos, SEEK_SET);
        break;
    case FILE_SEEK_CUR:
        fseek(file->fp, pos, SEEK_CUR);
        break;
    case FILE_SEEK_END:
        fseek(file->fp, pos, SEEK_END);
        break;
    }
}

size_t File_Pos(MYFILE *file)
{
    return ftell(file->fp);
}

size_t File_Size(MYFILE *file)
{
    size_t old = ftell(file->fp);
    fseek(file->fp, 0, SEEK_END);
    size_t size = ftell(file->fp);
    fseek(file->fp, old, SEEK_SET);
    return size;
}

const char *File_GetPath(MYFILE *file)
{
    return file->path;
}

void File_Close(MYFILE *file)
{
    fclose(file->fp);
    Memory_FreePointer(&file->path);
    // free per-file line buffer
    Memory_FreePointer(&file);
}

bool File_Load(const char *path, char **output_data, size_t *output_size)
{
    ASSERT(output_data != nullptr);

    MYFILE *fp = File_Open(path, FILE_OPEN_READ);
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
    char *full_path = File_GetFullPath(path);
    ASSERT(full_path != nullptr);
#if defined(_WIN32)
    _mkdir(full_path);
#else
    mkdir(full_path, 0775);
#endif
    Memory_FreePointer(&full_path);
}

void File_EnsureParentDirectories(const char *path)
{
    ASSERT(path != nullptr);
    LOG_INFO("%s", path);
    char *parent = File_GetParentDirectory(path);
    LOG_INFO("parent: %s", parent);
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
    char *full_path = File_GetFullPath(path);
    DIR *path_dir = opendir(full_path);
    Memory_FreePointer(&full_path);
    return path_dir;
}

const char *File_ReadDirectory(void *const dir)
{
    DIR *path_dir = (DIR *)dir;
    struct dirent *cur_file = readdir(dir);
    if (cur_file == nullptr) {
        return nullptr;
    }
    return cur_file->d_name;
}

void File_CloseDirectory(void *const dir)
{
    ASSERT(dir != nullptr);
    closedir(dir);
}
