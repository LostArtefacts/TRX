#include "game/item_actions/finish_level.h"

#include <libtrx/game/game.h>

void ItemAction_FinishLevel(ITEM *item)
{
    Game_SetIsLevelComplete(true);
}
