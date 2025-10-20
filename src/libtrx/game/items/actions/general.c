#include "game/game.h"
#include "game/items.h"

static void M_FinishLevel(ITEM *const item)
{
    Game_SetIsLevelComplete(true);
}

REGISTER_ITEM_ACTION(ITEM_ACTION_FINISH_LEVEL, M_FinishLevel)
