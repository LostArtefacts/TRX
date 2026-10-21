#pragma once

#include <trx/core/math/types.h>

#include <stdint.h>

// Fires a TR4 crossbow bolt of the kind the crossbow is loaded with, from a
// place, flying the way rot points. Returns the bolt's item number, or
// NO_ITEM where no item is free.
int16_t CrossbowBolt_Fire(XYZ_32 pos, int16_t room_num, XYZ_16 rot);
