#include "game/objects/general/trapdoor.h"

int32_t __cdecl Trapdoor_IsItemOnTop(
    const ITEM *const item, const int32_t x, const int32_t z)
{
    // trapdoor sector
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
        return test.x == obj.x && (test.z == obj.z || test.z == obj.z + 1);

    case -PHD_180:
        return test.x == obj.x && (test.z == obj.z || test.z == obj.z - 1);

    case PHD_90:
        return test.z == obj.z && (test.x == obj.x || test.x == obj.x + 1);

    case -PHD_90:
        return test.z == obj.z && (test.x == obj.x || test.x == obj.x - 1);
    }

    return false;
}
