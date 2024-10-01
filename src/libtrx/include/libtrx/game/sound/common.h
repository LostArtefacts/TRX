#pragma once

#include "../math.h"
#include "ids.h"

// clang-format off
typedef enum {
    SPM_NORMAL     = 0,
    SPM_UNDERWATER = 1,
    SPM_ALWAYS     = 2,
} SOUND_PLAY_MODE;
// clang-format on

bool Sound_Effect(SOUND_EFFECT_ID sfx_num, const XYZ_32 *pos, uint32_t flags);
bool Sound_IsAvailable(SOUND_EFFECT_ID sfx_num);
