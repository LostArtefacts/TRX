#ifndef T1M_SPECIFIC_FILE_H
#define T1M_SPECIFIC_FILE_H

#include <stddef.h>
#include <stdint.h>

int32_t LoadLevel(const char *filename, int32_t level_num);
int32_t S_LoadLevel(int32_t level_num);
const char *GetFullPath(const char *filename);
void FileLoad(const char *path, char **output_data, size_t *output_size);

void T1MInjectSpecificFile();

#endif
