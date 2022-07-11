#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    FILE_SEEK_SET,
    FILE_SEEK_CUR,
    FILE_SEEK_END,
} FILE_SEEK_MODE;

typedef enum {
    FILE_OPEN_READ,
    FILE_OPEN_READ_WRITE,
    FILE_OPEN_WRITE,
} FILE_OPEN_MODE;

typedef struct MYFILE MYFILE;

bool File_IsAbsolute(const char *path);

bool File_IsRelative(const char *path);

bool File_Exists(const char *path);

const char *File_GetGameDirectory(void);

// Get the absolute path to the given file, if possible.
// Internaly all operations on files within filesystem.c
// perform this normalization, so calling this function should
// only be necessary when interacting with external libraries.
char *File_GetFullPath(const char *path);

char *File_GuessExtension(const char *path, const char **extensions);

MYFILE *File_Open(const char *path, FILE_OPEN_MODE mode);

size_t File_Read(void *data, size_t item_size, size_t count, MYFILE *file);

size_t File_Write(
    const void *data, size_t item_size, size_t count, MYFILE *file);

void File_CreateDirectory(const char *path);

size_t File_Pos(MYFILE *file);

size_t File_Size(MYFILE *file);

const char *File_GetPath(MYFILE *file);

void File_Skip(MYFILE *file, size_t bytes);

void File_Seek(MYFILE *file, size_t pos, FILE_SEEK_MODE mode);

void File_Close(MYFILE *file);

bool File_Load(const char *path, char **output_data, size_t *output_size);
