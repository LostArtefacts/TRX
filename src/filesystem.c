#include "filesystem.h"

#include "log.h"
#include "memory.h"
#include "specific/s_filesystem.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MYFILE {
    FILE *fp;
};

bool File_IsAbsolute(const char *path)
{
    return path && (path[0] == '/' || strstr(path, ":\\"));
}

bool File_IsRelative(const char *path)
{
    return path && !File_IsAbsolute(path);
}

const char *File_GetGameDirectory()
{
    return S_File_GetGameDirectory();
}

bool File_Exists(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
}

void File_GetFullPath(const char *path, char **out)
{
    if (!File_Exists(path) && File_IsRelative(path)) {
        const char *game_path = File_GetGameDirectory();
        if (game_path) {
            size_t target_size = strlen(game_path) + 1 + strlen(path) + 1;
            *out = Memory_Alloc(target_size);
            strcpy(*out, game_path);
            strcat(*out, "\\");
            strcat(*out, path);
            return;
        }
    }
    *out = Memory_Dup(path);
    assert(*out);
}

void File_GuessExtension(const char *path, char **out, const char **extensions)
{
    if (!File_Exists(path)) {
        const char *dot = strrchr(path, '.');
        if (dot) {
            for (const char **ext = &extensions[0]; *ext; ext++) {
                size_t target_size = dot - path + strlen(*ext) + 1;
                *out = Memory_Alloc(target_size);
                strncpy(*out, path, dot - path);
                out[dot - path] = '\0';
                strcat(*out, *ext);
                if (File_Exists(*out)) {
                    return;
                }
                Memory_FreePointer(out);
            }
        }
    }
    *out = Memory_Dup(path);
    assert(*out);
}

MYFILE *File_Open(const char *path, FILE_OPEN_MODE mode)
{
    char *full_path = NULL;
    File_GetFullPath(path, &full_path);
    MYFILE *file = Memory_Alloc(sizeof(MYFILE));
    switch (mode) {
    case FILE_OPEN_WRITE:
        file->fp = fopen(full_path, "wb");
        break;
    case FILE_OPEN_READ:
        file->fp = fopen(full_path, "rb");
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
    S_File_CreateDirectory(path);
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

size_t File_Size(MYFILE *file)
{
    size_t old = ftell(file->fp);
    fseek(file->fp, 0, SEEK_END);
    size_t size = ftell(file->fp);
    fseek(file->fp, old, SEEK_SET);
    return size;
}

void File_Close(MYFILE *file)
{
    fclose(file->fp);
    Memory_FreePointer(&file);
}

int File_Delete(const char *path)
{
    return remove(path);
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
