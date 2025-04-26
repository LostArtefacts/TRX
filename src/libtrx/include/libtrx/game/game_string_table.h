#pragma once

#include <stdint.h>

void GameStringTable_Init(void);
void GameStringTable_Shutdown(void);

void GameStringTable_Load(const char *path, bool load_levels);
void GameStringTable_Apply(const GF_LEVEL *level);
