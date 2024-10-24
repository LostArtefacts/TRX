#pragma once

#include "global/types.h"

int32_t __cdecl Drawbridge_IsItemOnTop(const ITEM *item, int32_t z, int32_t x);

void __cdecl Drawbridge_Floor(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int32_t *out_height);
