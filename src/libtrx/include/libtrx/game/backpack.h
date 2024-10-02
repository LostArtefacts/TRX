#pragma once

#include "objects/ids.h"

#include <stdbool.h>
#include <stdint.h>

bool Backpack_AddItem(GAME_OBJECT_ID object_id);
bool Backpack_AddItemNTimes(GAME_OBJECT_ID object_id, int32_t n);
