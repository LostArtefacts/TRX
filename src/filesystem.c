#include "filesystem.h"

#include "log.h"
#include "memory.h"
#include "specific/s_main.h"

#include <assert.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>

struct MYFILE {
    FILE *fp;
};

bool File_IsRelative(const char *path)
{
    return PathIsRelativeA(path);
}

const char *File_GetGameDirectory()
{
    HMODULE module = NULL;
    DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
        | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;

    if (!GetModuleHandleEx(flags, NULL, &module)) {
        LOG_ERROR("Can't get module handle");
        return NULL;
    }

    static char path[MAX_PATH];
    GetModuleFileNameA(module, path, sizeof(path));
    PathRemoveFileSpecA(path);

    return path;
}

MYFILE *FileOpen(const char *path, FILE_OPEN_MODE mode)
{
    MYFILE *file = Memory_Alloc(sizeof(MYFILE));
    switch (mode) {
    case FILE_OPEN_WRITE:
        file->fp = fopen(path, "wb");
        break;
    case FILE_OPEN_READ:
        file->fp = fopen(path, "rb");
        break;
    default:
        file->fp = NULL;
        break;
    }
    if (!file->fp && File_IsRelative(path)) {
        const char *game_path = File_GetGameDirectory();
        if (game_path) {
            assert(strlen(game_path) + 1 + strlen(path) < MAX_PATH);
            char new_path[MAX_PATH] = "";
            strcpy(new_path, game_path);
            strcat(new_path, "\\");
            strcat(new_path, path);
            switch (mode) {
            case FILE_OPEN_WRITE:
                file->fp = fopen(new_path, "wb");
                break;
            case FILE_OPEN_READ:
                file->fp = fopen(new_path, "rb");
                break;
            default:
                file->fp = NULL;
                break;
            }
        }
    }
    if (!file->fp) {
        Memory_Free(file);
        return NULL;
    }
    return file;
}

size_t FileRead(void *data, size_t item_size, size_t count, MYFILE *file)
{
    return fread(data, item_size, count, file->fp);
}

size_t FileWrite(const void *data, size_t item_size, size_t count, MYFILE *file)
{
    return fwrite(data, item_size, count, file->fp);
}

void FileSeek(MYFILE *file, size_t pos, FILE_SEEK_MODE mode)
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

size_t FileSize(MYFILE *file)
{
    size_t old = ftell(file->fp);
    fseek(file->fp, 0, SEEK_END);
    size_t size = ftell(file->fp);
    fseek(file->fp, old, SEEK_SET);
    return size;
}

void FileClose(MYFILE *file)
{
    fclose(file->fp);
    Memory_Free(file);
}

int FileDelete(const char *path)
{
    return remove(path);
}

void FileLoad(const char *path, char **output_data, size_t *output_size)
{
    MYFILE *fp = FileOpen(path, FILE_OPEN_READ);
    if (!fp) {
        ShowFatalError("File load error");
        return;
    }

    size_t data_size = FileSize(fp);
    char *data = Memory_Alloc(data_size);
    if (!data) {
        ShowFatalError("Failed to allocate memory");
        return;
    }
    if (FileRead(data, sizeof(char), data_size, fp) != data_size) {
        ShowFatalError("File read error");
        return;
    }
    FileClose(fp);

    *output_data = data;
    *output_size = data_size;
}
