#include "game/ui/dialogs/stats.h"

#include "game/gym.h"

void UI_StatsDialog_Init(
    UI_STATS_DIALOG_STATE *const s, const UI_STATS_DIALOG_ARGS args)
{
    const ASSAULT_STATS stats = Gym_GetAssaultStats();
    int32_t max_assault_times = 0;
    for (int i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (stats.best_time[i] != 0) {
            max_assault_times++;
        }
    }
    UI_Requester_Init(&s->assault_req, 7, max_assault_times, false);
    s->assault_req.reserve_space = true;
    s->args = args;
}

void UI_StatsDialog_Free(UI_STATS_DIALOG_STATE *const s)
{
    UI_Requester_Free(&s->assault_req);
}

int32_t UI_StatsDialog_Control(UI_STATS_DIALOG_STATE *const s)
{
    return UI_Requester_Control(&s->assault_req);
}
