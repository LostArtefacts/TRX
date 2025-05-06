#include "game/item_actions/flipmap.h"

#include <libtrx/game/rooms.h>

void ItemAction_FlipMap(ITEM *item)
{
    Room_FlipMap();
}
