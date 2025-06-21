#include "game/option/common.h"

#include "game/option/controls.h"
#include "game/option/gameplay.h"
#include "game/option/graphics.h"
#include "game/option/sound.h"

void Option_Reset(void)
{
    Option_Shutdown();
}

void Option_Shutdown(void)
{
    Option_Gameplay_Shutdown();
    Option_Graphics_Shutdown();
    Option_Sound_Shutdown();
    Option_Controls_Shutdown();
}
