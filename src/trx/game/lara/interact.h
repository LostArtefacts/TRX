#pragma once

#include <trx/game/lara/enum.h>

#include <stdint.h>

bool Lara_Interact_CanBegin(LARA_INTERACT_MODE mode);
bool Lara_Interact_CanControl(LARA_INTERACT_MODE mode, int16_t item_num);
