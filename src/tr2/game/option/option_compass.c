#include "decomp/stats.h"
#include "game/input.h"
#include "game/option/option.h"
#include "game/requester.h"
#include "game/sound.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <stdio.h>

void __cdecl Option_Compass(INVENTORY_ITEM *const item)
{
    char buffer[32];
    const int32_t sec = g_SaveGame.statistics.timer / FRAMES_PER_SECOND;
    sprintf(buffer, "%02d:%02d:%02d", sec / 3600, sec / 60 % 60, sec % 60);

    if (g_CurrentLevel == LV_GYM) {
        ShowGymStatsText(buffer, 1);
    } else {
        ShowStatsText(buffer, 1);
    }

    if ((g_InputDB & IN_SELECT) || (g_InputDB & IN_DESELECT)) {
        item->anim_direction = 1;
        item->goal_frame = item->frames_total - 1;
    }

    Sound_Effect(SFX_MENU_STOPWATCH, 0, SPM_ALWAYS);
}

void Option_Compass_Shutdown(void)
{
    Requester_Shutdown(&g_StatsRequester);
}
