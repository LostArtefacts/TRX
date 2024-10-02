#include "game/backpack.h"

bool Backpack_AddItemNTimes(const GAME_OBJECT_ID object_id, const int32_t n)
{
    bool result = false;
    for (int32_t i = 0; i < n; i++) {
        result |= Backpack_AddItem(object_id);
    }
    return result;
}
