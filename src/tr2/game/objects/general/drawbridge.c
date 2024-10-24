#include "game/objects/general/drawbridge.h"

int32_t __cdecl Drawbridge_IsItemOnTop(
    const ITEM *const item, const int32_t z, const int32_t x)
{
    // drawbridge sector
    const XZ_32 obj = {
        .x = item->pos.x >> WALL_SHIFT,
        .z = item->pos.z >> WALL_SHIFT,
    };

    // test sector
    const XZ_32 test = {
        .x = x >> WALL_SHIFT,
        .z = z >> WALL_SHIFT,
    };

    switch (item->rot.y) {
    case 0:
        return test.x == obj.x && (test.z == obj.z - 1 || test.z == obj.z - 2);

    case -PHD_180:
        return test.x == obj.x && (test.z == obj.z + 1 || test.z == obj.z + 2);

    case -PHD_90:
        return test.z == obj.z && (test.x == obj.x + 1 || test.x == obj.x + 2);

    case PHD_90:
        return test.z == obj.z && (test.x == obj.x - 1 || test.x == obj.x - 2);
    }

    return false;
}
