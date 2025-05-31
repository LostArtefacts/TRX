#pragma once

// A tab switch UI element for navigating between multiple tabs via left/right
// input.

#include "../../game_string.h"
#include "../common.h"
#include "../scrollable.h"

// Represents a single tab page for use with UI_TabSwitch_Control.
typedef struct {
    GAME_STRING_ID header;
} UI_TAB_SWITCH_TAB;

typedef struct {
    UI_TAB_SWITCH_TAB *tabs;
    int32_t tab_count;
    int32_t active_tab_idx;
} UI_TAB_SWITCH_STATE;

// state functions
UI_TAB_SWITCH_STATE *UI_TabSwitch_Init(
    int32_t tab_count, const UI_TAB_SWITCH_TAB *tabs);
void UI_TabSwitch_Free(UI_TAB_SWITCH_STATE *s);

// Handles left/right input for switching tabs. Returns true if the active tab
// changed.
bool UI_TabSwitch_Control(UI_TAB_SWITCH_STATE *state);

// Advances the active tab by dir (-1 for previous, +1 for next), wrapping
// around.
void UI_TabSwitch_Cycle(UI_TAB_SWITCH_STATE *state, int32_t dir);

// draw functions
void UI_TabSwitch(const UI_TAB_SWITCH_STATE *state, bool is_focused);
