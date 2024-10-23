#include "game/objects/general/bridge_tilt_1.h"

#include "global/funcs.h"

void __cdecl BridgeTilt1_Floor(
    ITEM *const item, const int32_t x, const int32_t y, const int32_t z,
    int32_t *const out_height)
{
    const int32_t offset_height =
        item->pos.y + (Bridge_GetOffset(item, x, z) / 4);

    if (y > offset_height) {
        return;
    }

    *out_height = offset_height;
}
