#pragma once

#include "global/types.h"

void __cdecl BridgeTilt1_Floor(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int32_t *out_height);

void __cdecl BridgeTilt1_Ceiling(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int32_t *out_height);
