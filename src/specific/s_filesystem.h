#pragma once

#include <stdbool.h>

const char *S_File_GetGameDirectory(void);
void S_File_CreateDirectory(const char *path);
char *S_File_CasePath(char const *path);
