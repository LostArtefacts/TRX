#include "game/clock.h"

#include <libtrx/config.h>

int32_t Clock_GetCurrentFPS(void)
{
    return g_Config.rendering.fps;
}
