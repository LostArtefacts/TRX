#pragma once

#include <trx/game/types.h>

// Releases a TR4 bubble that rises to the water surface. The bubble starts
// at size plus a random amount masked by biggest, and nothing is released
// outside water.
void FX_Bubble_Create(
    XYZ_32 pos, int16_t room_num, int32_t size, int32_t biggest);
