#pragma once

// Public Lara routines.

#include "game/game_flow/types.h"
#include "global/types.h"

#include <libtrx/game/lara/common.h>

#include <stdint.h>

void Lara_Control(void);

ITEM *Lara_GetDeathCameraTarget(void);
void Lara_SetDeathCameraTarget(int16_t item_num);

void Lara_ControlExtra(int16_t item_num);
void Lara_AnimateUntil(ITEM *lara_item, int32_t goal);

void Lara_Initialise(const GF_LEVEL *level);
void Lara_InitialiseLoad(int16_t item_num);
void Lara_InitialiseInventory(const GF_LEVEL *level);
void Lara_InitialiseMeshes(const GF_LEVEL *level);

void Lara_SwapMeshExtra(void);
void Lara_UseItem(GAME_OBJECT_ID obj_id);

void Lara_RevertToPistolsIfNeeded(void);
