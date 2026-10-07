#pragma once

#include <trx/core/math/types.h>
#include <trx/game/items/types.h>
#include <trx/game/lara/enum.h>
#include <trx/game/objects/types.h>

#include <stdint.h>

bool Lara_Interact_HasActiveTarget(int16_t item_num);
bool Lara_Interact_HasActiveType(LARA_INTERACT_MODE mode);
bool Lara_Interact_CanBegin(LARA_INTERACT_MODE mode);
bool Lara_Interact_CanControl(LARA_INTERACT_MODE mode, int16_t item_num);
void Lara_Interact_FinishControl(LARA_INTERACT_MODE mode);

// Ends Lara's walk to the item and frees her hands, if she was walking to it.
void Lara_Interact_Release(int16_t item_num);

// Walks Lara one step to the position relative to the item, and returns
// whether she is there. Until then, the item stays her target.
bool Lara_Interact_MoveTo(const ITEM *item, const XYZ_32 *position);

// Walks Lara to the position while she stays within the item's bounds, and
// releases her when she leaves them.
LARA_REACH Lara_Interact_Reach(
    const ITEM *item, const OBJECT_BOUNDS *bounds, const XYZ_32 *position);
