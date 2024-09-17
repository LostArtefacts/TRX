#include "game_string.h"

#include <libtrx/game/game_string.h>

void GameString_Init(void)
{
    // IWYU pragma: begin_keep
#include "game_string.def"
    // IWYU pragma: end_keep
}

void GameString_Shutdown(void)
{
    GameString_Clear();
}
