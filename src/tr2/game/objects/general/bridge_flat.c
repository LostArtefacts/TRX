#include "game/objects/general/bridge_flat.h"

void __cdecl BridgeFlat_Floor(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    int32_t *const out_height)
{
    if (y <= item->pos.y) {
        *out_height = item->pos.y;
    }
}

void __cdecl BridgeFlat_Ceiling(
    const ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    int32_t *const out_height)
{
    if (y > item->pos.y) {
        *out_height = item->pos.y + STEP_L;
    }
}
