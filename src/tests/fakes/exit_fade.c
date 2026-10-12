// Leaves the game-exit fade out of tests that draw an interface but never exit.

#include <trx/game/ui/exit_fade.h>

void UI_BeginExitFade(void)
{
}

bool UI_IsExitFading(void)
{
    return false;
}

void UI_DrawExitFade(void)
{
}
