#pragma once

#include <trx/game/anims/types.h>
#include <trx/game/items/types.h>

#include <stdint.h>

const ANIM *Lara_Rope_GetSwingAnim(void);
void Lara_Rope_ApplyVelocity(int16_t angle, uint16_t vel);
void Lara_Rope_UpdateSwing(ITEM *item);
void Lara_Rope_JumpOff(ITEM *item);
void Lara_Rope_FallOff(ITEM *item);
