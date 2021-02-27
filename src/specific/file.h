#ifndef T1M_SPECIFIC_FILE_H
#define T1M_SPECIFIC_FILE_H

#include <stdio.h>

// clang-format off
#define _fread                  ((size_t        (*)(void *, size_t, size_t, FILE *))0x00442C20)
#define LoadLevel               ((int32_t       (*)(const char *path, int32_t level_num))0x0041AFB0)
// clang-format on

int32_t LoadRooms(FILE* fp);
int32_t LoadObjects(FILE* fp);
int32_t LoadItems(FILE* handle);
int32_t S_LoadLevel(int32_t level_num);
const char* GetFullPath(const char* filename);
void FindCdDrive();

#ifdef T1M_FEAT_OG_FIXES
void FixPyramidSecretTrigger();
int32_t GetSecretCount();
#endif

void T1MInjectSpecificFile();

#endif
