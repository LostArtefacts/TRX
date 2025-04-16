#pragma once

#include "../common.h"
#include "../elements/requester.h"

typedef enum {
    UI_STATS_DIALOG_MODE_LEVEL,
    UI_STATS_DIALOG_MODE_FINAL,
#if TR_VERSION == 2
    UI_STATS_DIALOG_MODE_ASSAULT_COURSE,
#endif
} UI_STATS_DIALOG_MODE;

typedef enum {
    UI_STATS_DIALOG_STYLE_BARE,
    UI_STATS_DIALOG_STYLE_BORDERED,
} UI_STATS_DIALOG_STYLE;

typedef struct {
    UI_STATS_DIALOG_MODE mode;
    UI_STATS_DIALOG_STYLE style;
    int32_t level_num;
} UI_STATS_DIALOG_ARGS;

typedef struct {
    UI_STATS_DIALOG_ARGS args;
    UI_REQUESTER_STATE assault_req;
} UI_STATS_DIALOG_STATE;

void UI_StatsDialog_Init(UI_STATS_DIALOG_STATE *s, UI_STATS_DIALOG_ARGS args);
void UI_StatsDialog_Free(UI_STATS_DIALOG_STATE *s);
int32_t UI_StatsDialog_Control(UI_STATS_DIALOG_STATE *s);

extern void UI_StatsDialog(UI_STATS_DIALOG_STATE *s);
