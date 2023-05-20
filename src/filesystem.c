#include "filesystem.h"

#include "log.h"
#include "memory.h"
#include "specific/s_filesystem.h"

#include <stdio.h>
#include <string.h>

struct MYFILE {
    FILE *fp;
    const char *path;
};

static void File_RemoveSubstring(char *str, const char *sub);
static bool File_ExistsRaw(const char *path);

static void File_RemoveSubstring(char *str, const char *sub)
{
    size_t len = strlen(sub);
    if (len == 0) {
        return;
    }

    char *p = str;
    while ((p = strstr(p, sub))) {
        memmove(p, p + len, strlen(p + len) + 1);
    }
}

static bool File_ExistsRaw(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
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
    return S_File_GetGameDirectory();
}

bool File_Exists(const char *path)
{
    char *full_path = File_GetFullPath(path);
    bool ret = File_ExistsRaw(full_path);
    Memory_FreePointer(&full_path);
    return ret;
}

char *File_GetFullPath(const char *path)
{
    if (!File_ExistsRaw(path) && File_IsRelative(path)) {
        const char *game_dir = File_GetGameDirectory();
        if (game_dir) {
            size_t out_size = strlen(game_dir) + strlen(path) + 2;
            char *out = Memory_Alloc(out_size);
            sprintf(out, "%s/%s", game_dir, path);
            if (!File_ExistsRaw(out)) {
                Memory_FreePointer(&out);
            } else {
                return out;
            }
        }
    }

    // Check for case sensitive path.
    if (File_IsRelative(path)) {
        const char *game_dir = File_GetGameDirectory();
        if (game_dir) {
            char *case_path = Memory_Alloc(strlen(path) + 2);
            if (S_File_CasePath(path, case_path)) {
                File_RemoveSubstring(case_path, "./");
            }
            size_t out_size = strlen(game_dir) + strlen(case_path) + 2;
            char *out = Memory_Alloc(out_size);
            sprintf(out, "%s%s", game_dir, case_path);
            Memory_FreePointer(&case_path);
            return out;
        }
    }
    return Memory_DupStr(path);
}

char *File_GuessExtension(const char *path, const char **extensions)
{
    if (!File_Exists(path)) {
        const char *dot = strrchr(path, '.');
        if (dot) {
            for (const char **ext = &extensions[0]; *ext; ext++) {
                size_t out_size = dot - path + strlen(*ext) + 1;
                char *out = Memory_Alloc(out_size);
                strncpy(out, path, dot - path);
                out[dot - path] = '\0';
                strcat(out, *ext);
                if (File_Exists(out)) {
                    return out;
                }
                Memory_FreePointer(&out);
            }
        }
    }
    return Memory_DupStr(path);
}

MYFILE *File_Open(const char *path, FILE_OPEN_MODE mode)
{
    LOG_DEBUG("path: %s", path);
    char *full_path = File_GetFullPath(path);
    MYFILE *file = Memory_Alloc(sizeof(MYFILE));
    file->path = Memory_DupStr(path);
    switch (mode) {
    case FILE_OPEN_WRITE:
        file->fp = fopen(full_path, "wb");
        break;
    case FILE_OPEN_READ:
        file->fp = fopen(full_path, "rb");
        break;
    case FILE_OPEN_READ_WRITE:
        file->fp = fopen(full_path, "r+b");
        break;
    default:
        file->fp = NULL;
        break;
    }
    Memory_FreePointer(&full_path);
    if (!file->fp) {
        Memory_FreePointer(&file);
    }
    return file;
}

size_t File_Read(void *data, size_t item_size, size_t count, MYFILE *file)
{
    return fread(data, item_size, count, file->fp);
}

size_t File_Write(
    const void *data, size_t item_size, size_t count, MYFILE *file)
{
    return fwrite(data, item_size, count, file->fp);
}

void File_CreateDirectory(const char *path)
{
    char *full_path = File_GetFullPath(path);
    S_File_CreateDirectory(full_path);
    Memory_FreePointer(&full_path);
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
    Memory_FreePointer(&file);
}

bool File_Load(const char *path, char **output_data, size_t *output_size)
{
    MYFILE *fp = File_Open(path, FILE_OPEN_READ);
    if (!fp) {
        LOG_ERROR("Can't open file %s", path);
        return false;
    }

    size_t data_size = File_Size(fp);
    char *data = Memory_Alloc(data_size + 1);
    if (File_Read(data, sizeof(char), data_size, fp) != data_size) {
        LOG_ERROR("Can't read file %s", path);
        Memory_FreePointer(&data);
        return false;
    }
    File_Close(fp);
    data[data_size] = '\0';

    *output_data = data;
    if (output_size) {
        *output_size = data_size;
    }
    return true;
}
