#include "game/game.h"
#include "game/items.h"
#include "game/rooms.h"

static void M_FinishLevel(ITEM *const item)
{
    Game_SetIsLevelComplete(true);
}

static void M_FlipMap(ITEM *const item)
{
    Room_FlipMap();
}

REGISTER_ITEM_ACTION(ITEM_ACTION_FINISH_LEVEL, M_FinishLevel)
REGISTER_ITEM_ACTION(ITEM_ACTION_FLIP_MAP, M_FlipMap)
