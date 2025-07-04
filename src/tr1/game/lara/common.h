#pragma once

// Public Lara routines.

#include "global/types.h"

#include <libtrx/game/lara/common.h>

void Lara_InitialiseLoad(int16_t item_num);

void Lara_SwapMeshExtra(void);
void Lara_UseItem(GAME_OBJECT_ID obj_id);

void Lara_RevertToPistolsIfNeeded(void);
