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
    UI_BeginHide(s->scroll.first_item == 0);
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
    UI_BeginHide(
        s->scroll.first_item + s->scroll.vis_items >= s->scroll.max_items);
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
    s->scroll = (UI_SCROLLABLE) {
        .first_item = 0,
        .sel_item = 0,
        .vis_items = vis_rows,
        .max_items = max_rows,
    };
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
            UI_Scrollable_SelectNext(&s->scroll, g_Config.ui.enable_wraparound);
        } else if (g_InputDB.menu_up) {
            UI_Scrollable_SelectPrev(&s->scroll, g_Config.ui.enable_wraparound);
        }
    } else {
        if (g_InputDB.menu_down) {
            UI_Scrollable_ScrollDown(&s->scroll, g_Config.ui.enable_wraparound);
        } else if (g_InputDB.menu_up) {
            UI_Scrollable_ScrollUp(&s->scroll, g_Config.ui.enable_wraparound);
        }
    }

    if (s->is_selectable) {
        if (g_InputDB.menu_back) {
            return UI_REQUESTER_CANCEL;
        }
        if (g_InputDB.menu_confirm) {
            return s->scroll.sel_item;
        }
    }
    return UI_REQUESTER_NO_CHOICE;
}

void UI_Requester_SetMaxRows(UI_REQUESTER_STATE *const s, const size_t max_rows)
{
    UI_Scrollable_SetMaxItems(&s->scroll, max_rows);
}

void UI_Requester_SetVisibleRows(
    UI_REQUESTER_STATE *const s, const size_t visible_rows)
{
    UI_Scrollable_SetVisibleItems(&s->scroll, visible_rows);
}

int32_t UI_Requester_GetFirstRow(const UI_REQUESTER_STATE *const s)
{
    return UI_Scrollable_GetFirstVisibleItem(&s->scroll);
}

int32_t UI_Requester_GetLastRow(const UI_REQUESTER_STATE *const s)
{
    return UI_Scrollable_GetLastVisibleItem(&s->scroll) + 1;
}

int32_t UI_Requester_GetCurrentRow(const UI_REQUESTER_STATE *s)
{
    return UI_Scrollable_GetSelectedItem(&s->scroll);
}

bool UI_Requester_IsRowVisible(
    const UI_REQUESTER_STATE *const s, const int32_t i)
{
    return i >= UI_Requester_GetFirstRow(s) && i < UI_Requester_GetLastRow(s);
}

bool UI_Requester_IsRowSelected(
    const UI_REQUESTER_STATE *const s, const int32_t i)
{
    return i == UI_Scrollable_GetSelectedItem(&s->scroll);
}

void UI_Requester_SelectRow(UI_REQUESTER_STATE *const s, const int32_t i)
{
    UI_Scrollable_SelectItem(&s->scroll, i);
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
            s->scroll.vis_items * TEXT_HEIGHT_FIXED
                + (s->scroll.vis_items - 1) * s->row_spacing);
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
