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
bool Lara_IsNearItem(const XYZ_32 *pos, int32_t distance);
void Lara_UseItem(GAME_OBJECT_ID obj_id);

bool Lara_TestPosition(const ITEM *item, const OBJECT_BOUNDS *bounds);
void Lara_AlignPosition(ITEM *item, XYZ_32 *vec);
bool Lara_MovePosition(ITEM *item, XYZ_32 *vec);

void Lara_RevertToPistolsIfNeeded(void);
