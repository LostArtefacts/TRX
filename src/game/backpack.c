#include "game/inventory.h"

#include <libtrx/game/backpack.h>

bool Backpack_AddItem(const GAME_OBJECT_ID object_id)
{
    return Inv_AddItem(object_id);
}
