#include "game/ui/elements/tab_switch.h"

#include "game/game_string.h"
#include "game/input/common.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/row_arrows.h"
#include "game/ui/elements/stack.h"
#include "game/ui/helpers.h"
#include "game/ui/scrollable.h"
#include "memory.h"

UI_TAB_SWITCH_STATE *UI_TabSwitch_Init(
    const int32_t tab_count, const UI_TAB_SWITCH_TAB *const tabs)
{
    UI_TAB_SWITCH_STATE *const s = Memory_Alloc(sizeof(UI_TAB_SWITCH_STATE));
    s->tab_count = tab_count;
    s->tabs = Memory_Dup(tabs, sizeof(UI_TAB_SWITCH_TAB) * tab_count);
    s->active_tab_idx = 0;
    return s;
}

void UI_TabSwitch_Free(UI_TAB_SWITCH_STATE *const s)
{
    Memory_Free(s->tabs);
    Memory_Free(s);
}

void UI_TabSwitch_Cycle(UI_TAB_SWITCH_STATE *const s, const int32_t dir)
{
    s->active_tab_idx += dir;
    if (s->active_tab_idx < 0) {
        s->active_tab_idx = s->tab_count - 1;
    } else if (s->active_tab_idx >= s->tab_count) {
        s->active_tab_idx = 0;
    }
}

bool UI_TabSwitch_Control(UI_TAB_SWITCH_STATE *const s)
{
    if (g_InputDB.menu_left) {
        UI_TabSwitch_Cycle(s, -1);
        return true;
    } else if (g_InputDB.menu_right) {
        UI_TabSwitch_Cycle(s, 1);
        return true;
    }
    return false;
}

void UI_TabSwitch(const UI_TAB_SWITCH_STATE *const s, const bool is_focused)
{
    UI_BeginAnchor(0.5f, 0.5f);
    UI_BeginRowArrows(is_focused, is_focused, UI_ROW_ARROWS_MEDIUM);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .align = { .h = UI_STACK_H_ALIGN_CENTER },
        .spacing = { .h = 10.0f },
    });
    for (int32_t i = 0; i < s->tab_count; i++) {
        const UI_TAB_SWITCH_TAB *const tab = &s->tabs[i];
        UI_BeginAnchor(0.5f, 0.5f);
        if (i == s->active_tab_idx) {
            UI_BeginFrame(
                is_focused ? UI_FRAME_SELECTED_OPTION : UI_FRAME_OUTLINE_ONLY);
        }
        UI_BeginPad(2.0f, 1.0f);
        UI_Label(GameString_Get(tab->header));
        UI_EndPad();
        if (i == s->active_tab_idx) {
            UI_EndFrame();
        }
        UI_EndAnchor();
    }
    UI_EndStack();
    UI_EndRowArrows();
    UI_EndAnchor();
}
