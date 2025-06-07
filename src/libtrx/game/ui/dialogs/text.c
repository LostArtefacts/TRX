#include "game/ui/dialogs/text.h"

#include "game/game_string.h"
#include "game/input.h"
#include "game/sound.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/resize.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <stdio.h>

#define TITLE_MARGIN 5.0f
#define DIALOG_PADDING 8.0f

static bool M_SelectPage(UI_TEXT_DIALOG_STATE *state, int32_t new_page);

static bool M_SelectPage(
    UI_TEXT_DIALOG_STATE *const state, const int32_t new_page)
{
    if (new_page == state->current_page || new_page < 0
        || new_page >= state->page_content->count) {
        return false;
    }
    state->current_page = new_page;
    return true;
}

void UI_TextDialog_Init(
    UI_TEXT_DIALOG_STATE *const state, const char *const title,
    const char *const text, const size_t width, const size_t max_lines,
    const bool is_heavy)
{
    state->current_page = 0;
    state->title = String_ToUpper(title);
    state->is_heavy = is_heavy;

    const char *wrapped = UI_Text_WordWrap(text, 1.0f, width);
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

void UI_TextDialog_Control(UI_TEXT_DIALOG_STATE *const state)
{
    const int32_t page_shift =
        g_InputDB.menu_left ? -1 : (g_InputDB.menu_right ? 1 : 0);
    if (M_SelectPage(state, state->current_page + page_shift)) {
        Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
    }
}

void UI_TextDialog_Free(UI_TEXT_DIALOG_STATE *const state)
{
    Memory_FreePointer(&state->title);
    for (int32_t i = state->page_content->count - 1; i >= 0; i--) {
        char *const page = *(char **)Vector_Get(state->page_content, i);
        Memory_Free(page);
    }
    Vector_Free(state->page_content);
}

void UI_TextDialog(UI_TEXT_DIALOG_STATE *const state)
{
    if (state->is_empty) {
        return;
    }

    UI_BeginModal(0.5f, 0.5f);
    UI_BeginFrame(
        state->is_heavy ? UI_FRAME_DIALOG_BACKGROUND_HEAVY
                        : UI_FRAME_DIALOG_BACKGROUND);
    UI_BeginPad(DIALOG_PADDING, DIALOG_PADDING);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
    });

    UI_BeginAnchor(0.5f, 0.5f);
    UI_Label(state->title);
    UI_EndAnchor();
    UI_Spacer(TITLE_MARGIN, TITLE_MARGIN);

    for (int32_t i = 0; i < state->page_content->count; i++) {
        if (i != state->current_page) {
            UI_BeginResize(-1.0f, 0.0f);
        } else if (state->page_content->count == 1) {
            UI_BeginResize(-1.0f, -1.0f);
        } else {
            UI_BeginResize(-1.0f, UI_TEXT_HEIGHT * state->max_lines);
        }
        UI_Label(*(char **)Vector_Get(state->page_content, i));
        UI_EndResize();
    }

    if (state->page_content->count > 1) {
        UI_Spacer(TITLE_MARGIN, TITLE_MARGIN * 3);

        UI_BeginAnchor(1.0f, 0.5f);
        UI_BeginStack(UI_STACK_HORIZONTAL);

        if (state->current_page > 0) {
            UI_Label("\\{button left} ");
        }

        char page_indicator[100];
        sprintf(
            page_indicator, GS(PAGINATION_NAV), state->current_page + 1,
            state->page_content->count);
        UI_Label(page_indicator);

        if (state->current_page < state->page_content->count - 1) {
            UI_Label(" \\{button right}");
        }

        UI_EndStack();
        UI_EndAnchor();
    }

    UI_EndStack();
    UI_EndPad();
    UI_EndFrame();
    UI_EndModal();
}
