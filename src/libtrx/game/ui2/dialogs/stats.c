#include "game/ui2/dialogs/stats.h"

void UI2_StatsDialog_Init(
    UI2_STATS_DIALOG_STATE *const s, const UI2_STATS_DIALOG_ARGS args)
{
    UI2_Requester_Init(&s->assault_req, 7, 10, false);
    s->args = args;
}

void UI2_StatsDialog_Free(UI2_STATS_DIALOG_STATE *const s)
{
    UI2_Requester_Free(&s->assault_req);
}

int32_t UI2_StatsDialog_Control(UI2_STATS_DIALOG_STATE *const s)
{
    return UI2_Requester_Control(&s->assault_req);
}
