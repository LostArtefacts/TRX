#include "game/objects/general/bridge_common.h"

int32_t __cdecl Bridge_GetOffset(
    const ITEM *const item, const int32_t x, const int32_t z)
{
    switch (item->rot.y) {
    case 0:
        return (WALL_L - x) & (WALL_L - 1);
    case -PHD_180:
        return x & (WALL_L - 1);
    case PHD_90:
        return z & (WALL_L - 1);
    default:
        return (WALL_L - z) & (WALL_L - 1);
    }
}
