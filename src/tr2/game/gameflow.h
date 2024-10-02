#pragma once

#include "global/types.h"

#include <stdint.h>

BOOL __cdecl GF_LoadFromFile(const char *file_name);
int32_t __cdecl GF_LoadScriptFile(const char *fname);
int32_t __cdecl GF_DoFrontendSequence(void);
int32_t __cdecl GF_DoLevelSequence(int32_t level, GAMEFLOW_LEVEL_TYPE type);
int32_t __cdecl GF_InterpretSequence(
    const int16_t *ptr, GAMEFLOW_LEVEL_TYPE type, int32_t seq_type);
void __cdecl GF_ModifyInventory(int32_t level, int32_t type);
