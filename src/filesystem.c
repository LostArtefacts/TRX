#include "filesystem.h"

#include "memory.h"
#include "specific/s_shell.h"
#include "specific/s_filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MYFILE {
    FILE *fp;
};

bool File_IsRelative(const char *path)
{
    return S_File_IsRelative(path);
}

const char *File_GetGameDirectory()
{
    return S_File_GetGameDirectory();
}

MYFILE *File_Open(const char *path, FILE_OPEN_MODE mode)
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
            char new_path[S_File_GetMaxPath()];
            if (strlen(game_path) + 1 + strlen(path) >= sizeof(new_path)) {
                S_Shell_ExitSystem("Too long path");
            }
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

size_t File_Read(void *data, size_t item_size, size_t count, MYFILE *file)
{
    return fread(data, item_size, count, file->fp);
}

size_t File_Write(
    const void *data, size_t item_size, size_t count, MYFILE *file)
{
    return fwrite(data, item_size, count, file->fp);
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
    Memory_Free(file);
}

int File_Delete(const char *path)
{
    return remove(path);
}

void File_Load(const char *path, char **output_data, size_t *output_size)
{
    MYFILE *fp = File_Open(path, FILE_OPEN_READ);
    if (!fp) {
        S_Shell_ExitSystem("File load error");
        return;
    }

    size_t data_size = File_Size(fp);
    char *data = Memory_Alloc(data_size);
    if (!data) {
        S_Shell_ExitSystem("Failed to allocate memory");
        return;
    }
    if (File_Read(data, sizeof(char), data_size, fp) != data_size) {
        S_Shell_ExitSystem("File read error");
        return;
    }
    File_Close(fp);

    *output_data = data;
    *output_size = data_size;
}
