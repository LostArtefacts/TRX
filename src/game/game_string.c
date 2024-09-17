#include "game_string.h"

#include <libtrx/game/game_string.h>

void GameString_Init(void)
{
#include "game_string.def"

#include <libtrx/game/game_string.def>
}

void GameString_Shutdown(void)
{
    GameString_Clear();
}
