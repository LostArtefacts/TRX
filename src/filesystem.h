#ifndef T1M_FILESYSTEM_H
#define T1M_FILESYSTEM_H

#include <stddef.h>

typedef enum {
    FILE_SEEK_SET,
    FILE_SEEK_CUR,
    FILE_SEEK_END,
} FILE_SEEK_MODE;

typedef enum {
    FILE_OPEN_READ,
    FILE_OPEN_WRITE,
} FILE_OPEN_MODE;

typedef struct MYFILE MYFILE;

MYFILE *FileOpen(const char *path, FILE_OPEN_MODE mode);
size_t FileRead(void *data, size_t item_size, size_t count, MYFILE *file);
size_t
FileWrite(const void *data, size_t item_size, size_t count, MYFILE *file);
size_t FileSize(MYFILE *file);
void FileSeek(MYFILE *file, size_t pos, FILE_SEEK_MODE mode);
void FileClose(MYFILE *file);
int FileDelete(const char *path);

#endif
