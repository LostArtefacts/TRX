#pragma once

#include <trx/game/output/mesh_batcher/batcher.h>
#include <trx/game/types.h>

void OutputSource_Shadows_Init(MESH_BATCHER *batcher);
void OutputSource_Shadows_Shutdown(void);

// Draws the shadow an item casts, in whichever style the player has chosen,
// on the floor under the place the item is drawn. Draws it once per frame
// however many times it is asked for.
void OutputSource_Shadows_Draw(
    int16_t size, const BOUNDS_16 *bounds, const ITEM *item);
