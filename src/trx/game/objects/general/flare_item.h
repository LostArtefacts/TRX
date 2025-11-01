#pragma once

#include <trx/game/math.h>

void Flare_GenerateEffects(
    const XYZ_32 *sound_pos, XYZ_32 flare_pos, int16_t room_num);
bool Flare_GenerateLight(XYZ_32 pos, int32_t flare_age);
int32_t Flare_GetMaxAge(void);
