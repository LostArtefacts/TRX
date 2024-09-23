#include "game/effect_routines/finish_level.h"

#include "global/vars.h"

#include <stdbool.h>

void FX_FinishLevel(ITEM *item)
{
    g_LevelComplete = true;
}
