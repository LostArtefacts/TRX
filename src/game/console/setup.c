#include "game/console/setup.h"

#include "game/console/cmd/easy_config.h"
#include "game/console/cmd/speed.h"

#include <libtrx/game/console/cmd/config.h>
#include <libtrx/game/console/cmd/die.h>
#include <libtrx/game/console/cmd/end_level.h>
#include <libtrx/game/console/cmd/exit_game.h>
#include <libtrx/game/console/cmd/exit_to_title.h>
#include <libtrx/game/console/cmd/flipmap.h>
#include <libtrx/game/console/cmd/fly.h>
#include <libtrx/game/console/cmd/give_item.h>
#include <libtrx/game/console/cmd/heal.h>
#include <libtrx/game/console/cmd/kill.h>
#include <libtrx/game/console/cmd/load_game.h>
#include <libtrx/game/console/cmd/play_demo.h>
#include <libtrx/game/console/cmd/play_level.h>
#include <libtrx/game/console/cmd/pos.h>
#include <libtrx/game/console/cmd/save_game.h>
#include <libtrx/game/console/cmd/set_health.h>
#include <libtrx/game/console/cmd/sfx.h>
#include <libtrx/game/console/cmd/teleport.h>

#include <stddef.h>

CONSOLE_COMMAND *g_ConsoleCommands[] = {
    &g_Console_Cmd_EasyConfig,
    &g_Console_Cmd_Teleport,
    &g_Console_Cmd_Fly,
    &g_Console_Cmd_Speed,
    &g_Console_Cmd_FlipMap,
    &g_Console_Cmd_Kill,
    &g_Console_Cmd_EndLevel,
    &g_Console_Cmd_PlayLevel,
    &g_Console_Cmd_LoadGame,
    &g_Console_Cmd_SaveGame,
    &g_Console_Cmd_PlayDemo,
    &g_Console_Cmd_ExitGame,
    &g_Console_Cmd_ExitToTitle,
    &g_Console_Cmd_Die,
    &g_Console_Cmd_Pos,
    &g_Console_Cmd_Heal,
    &g_Console_Cmd_SetHealth,
    &g_Console_Cmd_Config,
    &g_Console_Cmd_GiveItem,
    &g_Console_Cmd_SFX,
    NULL,
};
