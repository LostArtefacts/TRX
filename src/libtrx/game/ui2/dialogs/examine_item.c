#include "game/ui2/dialogs/examine_item.h"

#include "game/game_string.h"
#include "game/input.h"
#include "game/sound.h"
#include "game/text.h"
#include "game/ui2/elements/anchor.h"
#include "game/ui2/elements/frame.h"
#include "game/ui2/elements/label.h"
#include "game/ui2/elements/modal.h"
#include "game/ui2/elements/pad.h"
#include "game/ui2/elements/resize.h"
#include "game/ui2/elements/spacer.h"
#include "game/ui2/elements/stack.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <stdio.h>

#define TITLE_MARGIN 5.0f
#define WINDOW_MARGIN 10.0f
#define DIALOG_PADDING 8.0f
#define PADDING_SCALED (3.5f * (DIALOG_PADDING + WINDOW_MARGIN))

static bool M_SelectPage(UI2_EXAMINE_ITEM_STATE *state, int32_t new_page);

static bool M_SelectPage(
    UI2_EXAMINE_ITEM_STATE *const state, const int32_t new_page)
{
    if (new_page == state->current_page || new_page < 0
        || new_page >= state->page_content->count) {
        return false;
    }
    state->current_page = new_page;
    return true;
}

void UI2_ExamineItem_Init(
    UI2_EXAMINE_ITEM_STATE *const state, const char *const title,
    const char *const text, size_t max_lines)
{
    state->current_page = 0;
    state->title = String_ToUpper(title);

    const char *wrapped =
        String_WordWrap(text, Text_GetMaxLineLength() - PADDING_SCALED);
    state->page_content = String_Paginate(wrapped, max_lines);
    state->is_empty = String_IsEmpty(text);
    Memory_FreePointer(&wrapped);

    // Compute actual maximum number of lines on any single page, which can be
    // less than the line cap.
    size_t max_vis_lines = 0;
    for (int32_t i = 0; i < state->page_content->count; i++) {
        size_t page_lines = 1;
        const char *c = *(char **)Vector_Get(state->page_content, i);
        while (*c != '\0') {
            page_lines += *c++ == '\n';
        }
        CLAMPL(max_vis_lines, page_lines);
    }
    state->max_lines = max_vis_lines;
}

void UI2_ExamineItem_Control(UI2_EXAMINE_ITEM_STATE *const state)
{
    const int32_t page_shift =
        g_InputDB.menu_left ? -1 : (g_InputDB.menu_right ? 1 : 0);
    if (M_SelectPage(state, state->current_page + page_shift)) {
        Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
    }
}

void UI2_ExamineItem_Free(UI2_EXAMINE_ITEM_STATE *const state)
{
    Memory_FreePointer(&state->title);
    for (int32_t i = state->page_content->count - 1; i >= 0; i--) {
        char *const page = *(char **)Vector_Get(state->page_content, i);
        Memory_Free(page);
    }
    Vector_Free(state->page_content);
}

void UI2_ExamineItem(UI2_EXAMINE_ITEM_STATE *const state)
{
    if (state->is_empty) {
        return;
    }

    UI2_BeginModal(0.5f, 0.5f);
    UI2_BeginFrame(UI2_FRAME_DIALOG_BACKGROUND);
    UI2_BeginPad(DIALOG_PADDING, DIALOG_PADDING);
    UI2_BeginStackEx((UI2_STACK_SETTINGS) {
        .orientation = UI2_STACK_VERTICAL,
        .align = { .h = UI2_STACK_H_ALIGN_SPAN },
    });

    UI2_BeginAnchor(0.5f, 0.5f);
    UI2_Label(state->title);
    UI2_EndAnchor();
    UI2_Spacer(TITLE_MARGIN, TITLE_MARGIN);

    for (int32_t i = 0; i < state->page_content->count; i++) {
        if (i != state->current_page) {
            UI2_BeginResize(-1.0f, 0.0f);
        } else if (state->page_content->count == 1) {
            UI2_BeginResize(-1.0f, -1.0f);
        } else {
            UI2_BeginResize(-1.0f, TEXT_HEIGHT_FIXED * state->max_lines);
        }
        UI2_Label(*(char **)Vector_Get(state->page_content, i));
        UI2_EndResize();
    }

    if (state->page_content->count > 1) {
        UI2_Spacer(TITLE_MARGIN, TITLE_MARGIN * 3);

        UI2_BeginAnchor(1.0f, 0.5f);
        UI2_BeginStack(UI2_STACK_HORIZONTAL);

        if (state->current_page > 0) {
            UI2_Label("\\{button left} ");
        }

        char page_indicator[100];
        sprintf(
            page_indicator, GS(PAGINATION_NAV), state->current_page + 1,
            state->page_content->count);
        UI2_Label(page_indicator);

        if (state->current_page < state->page_content->count - 1) {
            UI2_Label(" \\{button right}");
        }

        UI2_EndStack();
        UI2_EndAnchor();
    }

    UI2_EndStack();
    UI2_EndPad();
    UI2_EndFrame();
    UI2_EndModal();
}
