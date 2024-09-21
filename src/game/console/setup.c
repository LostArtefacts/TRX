#include "game/console/setup.h"

#include "game/console/cmd/braid.h"
#include "game/console/cmd/cheats.h"
#include "game/console/cmd/end_level.h"
#include "game/console/cmd/exit_game.h"
#include "game/console/cmd/exit_to_title.h"
#include "game/console/cmd/flipmap.h"
#include "game/console/cmd/fps.h"
#include "game/console/cmd/load_game.h"
#include "game/console/cmd/play_demo.h"
#include "game/console/cmd/play_level.h"
#include "game/console/cmd/save_game.h"
#include "game/console/cmd/speed.h"
#include "game/console/cmd/teleport.h"
#include "game/console/cmd/vsync.h"
#include "game/console/cmd/wireframe.h"

#include <libtrx/game/console/cmd/config.h>
#include <libtrx/game/console/cmd/die.h>
#include <libtrx/game/console/cmd/fly.h>
#include <libtrx/game/console/cmd/give_item.h>
#include <libtrx/game/console/cmd/heal.h>
#include <libtrx/game/console/cmd/kill.h>
#include <libtrx/game/console/cmd/pos.h>
#include <libtrx/game/console/cmd/set_health.h>

#include <stddef.h>

CONSOLE_COMMAND *g_ConsoleCommands[] = {
    &g_Console_Cmd_Braid,       &g_Console_Cmd_FPS,
    &g_Console_Cmd_VSync,       &g_Console_Cmd_Wireframe,
    &g_Console_Cmd_Cheats,      &g_Console_Cmd_Teleport,
    &g_Console_Cmd_Fly,         &g_Console_Cmd_Speed,
    &g_Console_Cmd_FlipMap,     &g_Console_Cmd_Kill,
    &g_Console_Cmd_EndLevel,    &g_Console_Cmd_PlayLevel,
    &g_Console_Cmd_LoadGame,    &g_Console_Cmd_SaveGame,
    &g_Console_Cmd_PlayDemo,    &g_Console_Cmd_ExitGame,
    &g_Console_Cmd_ExitToTitle, &g_Console_Cmd_Die,
    &g_Console_Cmd_Pos,         &g_Console_Cmd_Heal,
    &g_Console_Cmd_SetHealth,   &g_Console_Cmd_Config,
    &g_Console_Cmd_GiveItem,    NULL,
};
