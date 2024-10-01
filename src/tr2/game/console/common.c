#include "game/console/common.h"

#include "game/console/setup.h"
#include "game/text.h"

int32_t Console_GetMaxLineLength(void)
{
    return TEXT_MAX_STRING_SIZE - 1;
}

CONSOLE_COMMAND **Console_GetCommands(void)
{
    return g_ConsoleCommands;
}

void Console_DrawBackdrop(void)
{
    // TODO: implement me
}
