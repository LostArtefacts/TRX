#include "game/lara/cheat.h"

#include "game/console.h"
#include "game/game.h"
#include "game/game_string.h"

void Lara_Cheat_EndLevel(void)
{
    Game_SetIsLevelComplete(true);
    Console_Log(GS(OSD_COMPLETE_LEVEL));
}
