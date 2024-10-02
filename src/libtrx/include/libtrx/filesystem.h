#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

bool File_DirExists(const char *path);

bool File_IsAbsolute(const char *path);

bool File_IsRelative(const char *path);

bool File_Exists(const char *path);

const char *File_GetGameDirectory(void);

// Get the absolute path to the given file, if possible.
// Internaly all operations on files within filesystem.c
// perform this normalization, so calling this function should
// only be necessary when interacting with external libraries.
char *File_GetFullPath(const char *path);

char *File_GetParentDirectory(const char *path);

char *File_GuessExtension(const char *path, const char **extensions);

MYFILE *File_Open(const char *path, FILE_OPEN_MODE mode);

void File_ReadData(MYFILE *file, void *data, size_t size);
void File_ReadItems(MYFILE *file, void *data, size_t count, size_t item_size);
int8_t File_ReadS8(MYFILE *file);
int16_t File_ReadS16(MYFILE *file);
int32_t File_ReadS32(MYFILE *file);
uint8_t File_ReadU8(MYFILE *file);
uint16_t File_ReadU16(MYFILE *file);
uint32_t File_ReadU32(MYFILE *file);

void File_WriteData(MYFILE *file, const void *data, size_t size);
void File_WriteItems(
    MYFILE *file, const void *data, size_t count, size_t item_size);
void File_WriteS8(MYFILE *file, int8_t value);
void File_WriteS16(MYFILE *file, int16_t value);
void File_WriteS32(MYFILE *file, int32_t value);
void File_WriteU8(MYFILE *file, uint8_t value);
void File_WriteU16(MYFILE *file, uint16_t value);
void File_WriteU32(MYFILE *file, uint32_t value);

size_t File_Pos(MYFILE *file);

size_t File_Size(MYFILE *file);

const char *File_GetPath(MYFILE *file);

void File_Skip(MYFILE *file, size_t bytes);

void File_Seek(MYFILE *file, size_t pos, FILE_SEEK_MODE mode);

void File_Close(MYFILE *file);

bool File_Load(const char *path, char **output_data, size_t *output_size);

void File_CreateDirectory(const char *path);
