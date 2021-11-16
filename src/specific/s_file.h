#ifndef T1M_SPECIFIC_S_FILE_H
#define T1M_SPECIFIC_S_FILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool LoadLevel(const char *filename, int32_t level_num);
bool S_LoadLevel(int32_t level_num);

#endif
