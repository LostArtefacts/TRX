#include "game/console/common.h"

#include "game/console/setup.h"
#include "game/text.h"

CONSOLE_COMMAND **Console_GetCommands(void)
{
    return g_ConsoleCommands;
}

void Console_DrawBackdrop(void)
{
    // TODO: implement me
}
