#include "game/ui2/elements/requester.h"

#include "game/input.h"
#include "game/ui2/elements/frame.h"
#include "game/ui2/elements/pad.h"
#include "game/ui2/elements/stack.h"
#include "game/ui2/elements/window.h"
#include "utils.h"

void UI2_Requester_Init(
    UI2_REQUESTER_STATE *const s, const int32_t vis_rows,
    const int32_t max_rows, const bool is_selectable)
{
    s->vis_row = 0;
    s->sel_row = 0;
    s->vis_rows = vis_rows;
    s->max_rows = max_rows;
    s->is_selectable = is_selectable;
    s->row_pad = 20.0f;
    s->row_spacing = 3.0f;
}

void UI2_Requester_Free(UI2_REQUESTER_STATE *const s)
{
}

int32_t UI2_Requester_Control(UI2_REQUESTER_STATE *const s)
{
    if (g_InputDB.menu_down) {
        if (s->vis_row + s->vis_rows < s->max_rows) {
            s->vis_row++;
        }
    } else if (g_InputDB.menu_up) {
        if (s->vis_row > 0) {
            s->vis_row--;
        }
    }

    if (s->is_selectable) {
        if (g_InputDB.menu_down && s->sel_row + 1 < s->max_rows) {
            s->sel_row++;
        } else if (g_InputDB.menu_up && s->sel_row > 0) {
            s->sel_row--;
        }
        if (g_InputDB.menu_back) {
            return UI2_REQUESTER_CANCEL;
        }
        if (g_InputDB.menu_confirm) {
            return s->sel_row;
        }
    }
    return UI2_REQUESTER_NO_CHOICE;
}

void UI2_Requester_SetMaxRows(
    UI2_REQUESTER_STATE *const s, const int32_t max_rows)
{
    s->max_rows = max_rows;
}

int32_t UI2_Requester_GetFirstRow(const UI2_REQUESTER_STATE *const s)
{
    return s->vis_row;
}

int32_t UI2_Requester_GetLastRow(const UI2_REQUESTER_STATE *const s)
{
    return MIN(s->vis_row + s->vis_rows, s->max_rows);
}

int32_t UI2_Requester_GetCurrentRow(const UI2_REQUESTER_STATE *s)
{
    return s->sel_row;
}

bool UI2_Requester_IsRowVisible(
    const UI2_REQUESTER_STATE *const s, const int32_t i)
{
    return i >= UI2_Requester_GetFirstRow(s)
        && i <= UI2_Requester_GetLastRow(s);
}

bool UI2_Requester_IsRowSelected(
    const UI2_REQUESTER_STATE *const s, const int32_t i)
{
    return i == s->sel_row;
}

void UI2_BeginRequester(
    const UI2_REQUESTER_STATE *const s, const char *const title)
{
    UI2_BeginWindow();
    UI2_WindowTitle(title);
    UI2_BeginWindowBody();

    UI2_BeginStackEx((UI2_STACK_SETTINGS) {
        .orientation = UI2_STACK_VERTICAL,
        .align = { .h = UI2_STACK_H_ALIGN_SPAN },
        .spacing = { .v = s->row_spacing },
    });
}

void UI2_EndRequester(const UI2_REQUESTER_STATE *const s)
{
    UI2_EndStack();
    UI2_EndWindowBody();
    UI2_EndWindow();
}

void UI2_BeginRequesterRow(const UI2_REQUESTER_STATE *const s, const int32_t i)
{
    UI2_BeginPad(0.0f, TR_VERSION == 1 ? -1.0f : 0.0f);
    if (UI2_Requester_IsRowSelected(s, i)) {
        UI2_BeginFrame(UI2_FRAME_SELECTED_OPTION);
    }
    UI2_BeginPad(s->row_pad, TR_VERSION == 1 ? 1.0f : 0.0f);
}

void UI2_EndRequesterRow(const UI2_REQUESTER_STATE *const s, const int32_t i)
{
    UI2_EndPad();
    if (UI2_Requester_IsRowSelected(s, i)) {
        UI2_EndFrame();
    }
    UI2_EndPad();
}
