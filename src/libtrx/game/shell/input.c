#include "game/shell/input.h"

#include "config.h"
#include "game/viewport.h"
#include "utils.h"

void Shell_IncreaseScreenSize(void)
{
    if (g_Config.rendering.borders < 0.45) {
        g_Config.rendering.borders += 0.05;
        CLAMPG(g_Config.rendering.borders, 0.45);
        Viewport_Reset();
    }
}

void Shell_DecreaseScreenSize(void)
{
    if (g_Config.rendering.borders > 0.0) {
        g_Config.rendering.borders -= 0.05;
        CLAMPL(g_Config.rendering.borders, 0.0);
        Viewport_Reset();
    }
}
