#pragma once

#include "../common.h"
#include "../elements/requester.h"

typedef enum {
    UI2_STATS_DIALOG_MODE_LEVEL,
    UI2_STATS_DIALOG_MODE_FINAL,
#if TR_VERSION == 2
    UI2_STATS_DIALOG_MODE_ASSAULT_COURSE,
#endif
} UI2_STATS_DIALOG_MODE;

typedef enum {
    UI2_STATS_DIALOG_STYLE_BARE,
    UI2_STATS_DIALOG_STYLE_BORDERED,
} UI2_STATS_DIALOG_STYLE;

typedef struct {
    UI2_STATS_DIALOG_MODE mode;
    UI2_STATS_DIALOG_STYLE style;
    int32_t level_num;
} UI2_STATS_DIALOG_ARGS;

typedef struct {
    UI2_STATS_DIALOG_ARGS args;
    UI2_REQUESTER_STATE assault_req;
} UI2_STATS_DIALOG_STATE;

void UI2_StatsDialog_Init(
    UI2_STATS_DIALOG_STATE *s, UI2_STATS_DIALOG_ARGS args);
void UI2_StatsDialog_Free(UI2_STATS_DIALOG_STATE *s);
int32_t UI2_StatsDialog_Control(UI2_STATS_DIALOG_STATE *s);

extern void UI2_StatsDialog(UI2_STATS_DIALOG_STATE *s);
