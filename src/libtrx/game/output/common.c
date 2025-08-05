#include "game/output/common.h"

#include "game/shell.h"

bool Output_IsHeadless(void)
{
    return Shell_GetArgs()->headless;
}
