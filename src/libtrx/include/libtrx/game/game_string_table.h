#pragma once

#include <stdint.h>

void GameStringTable_LoadFromFile(const char *path);
void GameStringTable_LoadFromString(const char *data);
void GameStringTable_Apply(const GF_LEVEL *level);
void GameStringTable_Shutdown(void);
