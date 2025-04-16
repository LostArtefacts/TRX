#include "game/ui/dialogs/stats.h"

void UI_StatsDialog_Init(
    UI_STATS_DIALOG_STATE *const s, const UI_STATS_DIALOG_ARGS args)
{
    UI_Requester_Init(&s->assault_req, 7, 10, false);
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
