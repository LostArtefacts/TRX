#pragma once

#include <trx/game/items/types.h>

void FX_UPVWake_Reset(void);

// Adds a wake segment between two points on one side, unless the next slot
// on that side is still in use. The wake then belongs to the given UPV.
void FX_UPVWake_Spawn(const ITEM *item, int32_t side, const XYZ_32 pos[2]);

// Fades the wake in while its UPV moves, and out while it stands still.
// Other UPVs leave the wake alone.
void FX_UPVWake_UpdateShade(const ITEM *item, bool is_moving);

// Draws the wake of one side as a strip that starts at the two origin points.
// Draws nothing for a UPV that does not own the wake.
void FX_UPVWake_Draw(const ITEM *item, int32_t side, const XYZ_32 origin[2]);
