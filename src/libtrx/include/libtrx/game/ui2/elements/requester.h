#pragma once

// A window to select a single option from a list of predefined choices.

#include "../common.h"

#include <stdint.h>

#define UI2_REQUESTER_CANCEL -2
#define UI2_REQUESTER_NO_CHOICE -1

typedef struct {
    bool is_selectable;
    int32_t vis_rows;
    int32_t max_rows;
    int32_t vis_row;
    int32_t sel_row;
    float row_pad;
    float row_spacing;
} UI2_REQUESTER_STATE;

// state functions
void UI2_Requester_Init(
    UI2_REQUESTER_STATE *s, int32_t vis_rows, int32_t max_rows,
    bool is_selectable);
void UI2_Requester_Free(UI2_REQUESTER_STATE *s);
int32_t UI2_Requester_Control(UI2_REQUESTER_STATE *s);
void UI2_Requester_SetMaxRows(UI2_REQUESTER_STATE *s, int32_t max_rows);
int32_t UI2_Requester_GetFirstRow(const UI2_REQUESTER_STATE *s);
int32_t UI2_Requester_GetLastRow(const UI2_REQUESTER_STATE *s);
int32_t UI2_Requester_GetCurrentRow(const UI2_REQUESTER_STATE *s);
bool UI2_Requester_IsRowVisible(const UI2_REQUESTER_STATE *s, int32_t i);
bool UI2_Requester_IsRowSelected(const UI2_REQUESTER_STATE *s, int32_t i);

// draw functions
void UI2_BeginRequester(const UI2_REQUESTER_STATE *s, const char *title);
void UI2_EndRequester(const UI2_REQUESTER_STATE *s);

void UI2_BeginRequesterRow(const UI2_REQUESTER_STATE *s, int32_t i);
void UI2_EndRequesterRow(const UI2_REQUESTER_STATE *s, int32_t i);
