#include "game/ui/elements/requester.h"

#include "config.h"
#include "game/input.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/stack.h"
#include "game/ui/elements/window.h"
#include "utils.h"

void UI_Requester_Init(
    UI_REQUESTER_STATE *const s, const int32_t vis_rows, const int32_t max_rows,
    const bool is_selectable)
{
    s->vis_row = 0;
    s->sel_row = 0;
    s->vis_rows = vis_rows;
    s->max_rows = max_rows;
    s->is_selectable = is_selectable;
    s->row_pad = 20.0f;
    s->row_spacing = 3.0f;
}

void UI_Requester_Free(UI_REQUESTER_STATE *const s)
{
}

int32_t UI_Requester_Control(UI_REQUESTER_STATE *const s)
{
    if (s->is_selectable) {
        if (g_InputDB.menu_down) {
            if (s->sel_row + 1 < s->max_rows) {
                s->sel_row++;
            } else if (g_Config.ui.enable_wraparound) {
                s->sel_row = 0;
            }
        } else if (g_InputDB.menu_up) {
            if (s->sel_row > 0) {
                s->sel_row--;
            } else if (g_Config.ui.enable_wraparound) {
                s->sel_row = s->max_rows - 1;
            }
        }
    }

    if (s->sel_row > s->vis_row + s->vis_rows) {
        s->vis_row++;
    }
    if (s->sel_row < s->vis_row) {
        s->vis_row = s->sel_row;
    }
    CLAMP(s->vis_row, 0, s->max_rows - s->vis_rows);

    if (s->is_selectable) {
        if (g_InputDB.menu_back) {
            return UI_REQUESTER_CANCEL;
        }
        if (g_InputDB.menu_confirm) {
            return s->sel_row;
        }
    }
    return UI_REQUESTER_NO_CHOICE;
}

void UI_Requester_SetMaxRows(
    UI_REQUESTER_STATE *const s, const int32_t max_rows)
{
    s->max_rows = max_rows;
}

void UI_Requester_SetVisibleRows(
    UI_REQUESTER_STATE *const s, const size_t visible_rows)
{
    s->vis_rows = visible_rows;
    CLAMP(s->vis_rows, 0, s->max_rows);
    if (s->sel_row != -1) {
        if (s->vis_row > s->sel_row) {
            s->vis_row = s->sel_row;
        } else if (s->sel_row > s->vis_row + s->vis_rows) {
            s->vis_row = s->sel_row - s->vis_rows + 1;
        }
    }

    CLAMP(s->vis_row, 0, s->max_rows - s->vis_rows);
}

int32_t UI_Requester_GetFirstRow(const UI_REQUESTER_STATE *const s)
{
    return s->vis_row;
}

int32_t UI_Requester_GetLastRow(const UI_REQUESTER_STATE *const s)
{
    return MIN(s->vis_row + s->vis_rows, s->max_rows);
}

int32_t UI_Requester_GetCurrentRow(const UI_REQUESTER_STATE *s)
{
    return s->sel_row;
}

bool UI_Requester_IsRowVisible(
    const UI_REQUESTER_STATE *const s, const int32_t i)
{
    return i >= UI_Requester_GetFirstRow(s) && i < UI_Requester_GetLastRow(s);
}

bool UI_Requester_IsRowSelected(
    const UI_REQUESTER_STATE *const s, const int32_t i)
{
    return i == s->sel_row;
}

void UI_BeginRequester(
    const UI_REQUESTER_STATE *const s, const char *const title)
{
    UI_BeginWindow();
    UI_WindowTitle(title);
    UI_BeginWindowBody();

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
        .spacing = { .v = s->row_spacing },
    });
}

void UI_EndRequester(const UI_REQUESTER_STATE *const s)
{
    UI_EndStack();
    UI_EndWindowBody();
    UI_EndWindow();
}

void UI_BeginRequesterRow(const UI_REQUESTER_STATE *const s, const int32_t i)
{
    UI_BeginPad(0.0f, TR_VERSION == 1 ? -1.0f : 0.0f);
    if (UI_Requester_IsRowSelected(s, i)) {
        UI_BeginFrame(UI_FRAME_SELECTED_OPTION);
    }
    UI_BeginPad(s->row_pad, TR_VERSION == 1 ? 1.0f : 0.0f);
}

void UI_EndRequesterRow(const UI_REQUESTER_STATE *const s, const int32_t i)
{
    UI_EndPad();
    if (UI_Requester_IsRowSelected(s, i)) {
        UI_EndFrame();
    }
    UI_EndPad();
}
