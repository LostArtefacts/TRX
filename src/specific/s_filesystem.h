#ifndef T1M_SPECIFIC_S_FILESYSTEM_H
#define T1M_SPECIFIC_S_FILESYSTEM_H

#include <stdbool.h>
#include <stdint.h>

size_t S_File_GetMaxPath();
bool S_File_IsRelative(const char *path);
const char *S_File_GetGameDirectory();

#endif
