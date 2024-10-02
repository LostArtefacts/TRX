#pragma once

#include "global/types.h"

#include <stdbool.h>

void __thiscall S_FlaggedString_Create(STRING_FLAGGED *string, int32_t size);
void __thiscall S_FlaggedString_Delete(STRING_FLAGGED *string);
void __thiscall S_FlaggedString_InitAdapter(DISPLAY_ADAPTER *adapter);
bool S_FlaggedString_Copy(STRING_FLAGGED *dst, STRING_FLAGGED *src);
