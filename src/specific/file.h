#ifndef T1M_SPECIFIC_FILE_H
#define T1M_SPECIFIC_FILE_H

#include <stdint.h>

int32_t LoadLevel(const char *filename, int32_t level_num);
int32_t S_LoadLevel(int32_t level_num);
const char *GetFullPath(const char *filename);
char *FileLoad(const char *path, char *target);

void T1MInjectSpecificFile();

#endif
