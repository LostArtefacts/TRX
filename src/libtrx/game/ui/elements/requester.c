#include "game/ui/elements/requester.h"

#include "config.h"
#include "game/input.h"
#include "game/text.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/fixed.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/hide.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/offset.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/resize.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "game/ui/elements/window.h"
#include "utils.h"

static void M_UpArrow(const UI_REQUESTER_STATE *s);
static void M_DownArrow(const UI_REQUESTER_STATE *s);

static void M_UpArrow(const UI_REQUESTER_STATE *const s)
{
    UI_BeginHide(s->vis_row == 0);
    UI_Spacer(0.0f, TR_VERSION == 2 ? 6.0f : 4.0f);
    UI_BeginAnchor(0.5f, 0.5f);
    UI_BeginFixed(0.5f, TR_VERSION == 2 ? 0.7f : 1.1f);
    UI_LabelEx("\\{arrow up}", (UI_LABEL_SETTINGS) { .scale = 0.7 });
    UI_EndFixed();
    UI_EndAnchor();
    UI_EndHide();
}

static void M_DownArrow(const UI_REQUESTER_STATE *const s)
{
    UI_BeginHide(s->vis_row + s->vis_rows >= s->max_rows);
    UI_BeginAnchor(0.5f, 0.0f);
    UI_BeginFixed(0.5f, -0.3f);
    UI_LabelEx("\\{arrow down}", (UI_LABEL_SETTINGS) { .scale = 0.7 });
    UI_EndFixed();
    UI_EndAnchor();
    UI_EndHide();
    UI_Spacer(0.0f, TR_VERSION == 2 ? 6.0f : 4.0f);
}

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
    s->show_arrows = false;
    s->reserve_space = false;
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
        CLAMP(s->vis_row, s->sel_row - s->vis_rows + 1, s->sel_row);
    } else {
        if (g_InputDB.menu_down) {
            if (s->vis_row + 1 <= s->max_rows - s->vis_rows) {
                s->vis_row++;
            } else if (g_Config.ui.enable_wraparound) {
                s->vis_row = 0;
            }
        } else if (g_InputDB.menu_up) {
            if (s->vis_row > 0) {
                s->vis_row--;
            } else if (g_Config.ui.enable_wraparound) {
                s->vis_row = s->max_rows - 1;
            }
        }
    }

    CLAMPG(s->vis_row, s->max_rows - s->vis_rows);
    CLAMPL(s->vis_row, 0);

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

void UI_Requester_SetMaxRows(UI_REQUESTER_STATE *const s, const size_t max_rows)
{
    s->max_rows = max_rows;
}

void UI_Requester_SetVisibleRows(
    UI_REQUESTER_STATE *const s, const size_t visible_rows)
{
    s->vis_rows = visible_rows;
    CLAMPL(s->vis_rows, 0);
    if (s->sel_row != -1) {
        CLAMP(s->vis_row, s->sel_row - s->vis_rows + 1, s->sel_row);
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
    });

    if (s->show_arrows) {
        M_UpArrow(s);
    }
    if (s->reserve_space) {
        UI_BeginResize(
            -1.0f,
            s->vis_rows * TEXT_HEIGHT_FIXED
                + (s->vis_rows - 1) * s->row_spacing);
    }

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
        .spacing = { .v = s->row_spacing },
    });
}

void UI_EndRequester(const UI_REQUESTER_STATE *const s)
{
    UI_EndStack();

    if (s->reserve_space) {
        UI_EndResize();
    }
    if (s->show_arrows) {
        M_DownArrow(s);
    }

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
