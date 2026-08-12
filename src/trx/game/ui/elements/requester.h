#pragma once

// A window to select a single option from a list of predefined choices.

#include <trx/game/ui/common.h>
#include <trx/game/ui/scrollable.h>

#include <stdint.h>

#define UI_REQUESTER_CANCEL -2
#define UI_REQUESTER_NO_CHOICE -1

typedef struct {
    bool is_selectable;
    UI_SCROLLABLE scroll;
    float row_pad;
    float row_spacing;
    bool show_arrows;
    bool reserve_space;
    // Height the dialog around the requester keeps for itself, such as a
    // button under the list.
    float footer_height;
} UI_REQUESTER_STATE;

// state functions
void UI_Requester_Init(
    UI_REQUESTER_STATE *s, int32_t vis_rows, int32_t max_rows,
    bool is_selectable);
void UI_Requester_Free(UI_REQUESTER_STATE *s);
int32_t UI_Requester_Control(UI_REQUESTER_STATE *s);
void UI_Requester_SetMaxRows(UI_REQUESTER_STATE *s, size_t max_rows);
void UI_Requester_SetVisibleRows(UI_REQUESTER_STATE *s, size_t visible_rows);
void UI_Requester_SelectRow(UI_REQUESTER_STATE *s, int32_t i);
int32_t UI_Requester_GetFirstRow(const UI_REQUESTER_STATE *s);
int32_t UI_Requester_GetLastRow(const UI_REQUESTER_STATE *s);
int32_t UI_Requester_GetCurrentRow(const UI_REQUESTER_STATE *s);
bool UI_Requester_IsRowVisible(const UI_REQUESTER_STATE *s, int32_t i);
bool UI_Requester_IsRowSelected(const UI_REQUESTER_STATE *s, int32_t i);

// The height the requester takes with the given number of rows: the rows
// themselves, the spacing between them, and the window around them.
float UI_Requester_GetHeight(const UI_REQUESTER_STATE *s, int32_t rows);

// The reverse: how many rows fit in the given height, never fewer than one.
int32_t UI_Requester_GetRowsForHeight(
    const UI_REQUESTER_STATE *s, float height);

// draw functions
void UI_BeginRequester(const UI_REQUESTER_STATE *s, const char *title);
void UI_EndRequester(const UI_REQUESTER_STATE *s);

void UI_BeginRequesterRow(const UI_REQUESTER_STATE *s, int32_t i);
void UI_EndRequesterRow(const UI_REQUESTER_STATE *s, int32_t i);
