#pragma once

#include "../../../config/option.h"
#include "../../../config/types.h"
#include "../../game_string.h"
#include "../common.h"
#include "../elements/tab_switch.h"
#include "../scrollable.h"
#include "./text.h"

typedef struct {
    int32_t value;
    GAME_STRING_ID name;
} UI_SETTINGS_ENUM_ENTRY;

typedef struct UI_SETTINGS_OPTION UI_SETTINGS_OPTION;

typedef struct {
    const char *(*format_value)(const struct UI_SETTINGS_OPTION *option);
    bool (*can_change_value)(
        const struct UI_SETTINGS_OPTION *option, int32_t dir);
    bool (*request_change_value)(
        const struct UI_SETTINGS_OPTION *option, int32_t dir);
    bool (*is_available)(const struct UI_SETTINGS_OPTION *option);
} UI_SETTINGS_CUSTOM_OPITON_HANDLER;

typedef struct UI_SETTINGS_OPTION {
    CONFIG_OPTION_TYPE option_type;
    GAME_STRING_ID label_id;
    GAME_STRING_ID description_id;

    // A custom handler that must have all the function pointers filled,
    UI_SETTINGS_CUSTOM_OPITON_HANDLER custom_handler;

    // ...or a convenience default handler options
    struct {
        void *target;
        int32_t min_value;
        int32_t max_value;
        int32_t delta_slow;
        int32_t delta_fast;
        const void *misc;
    };
} UI_SETTINGS_OPTION;

typedef enum {
    UI_SETTINGS_PHASE_NAVIGATE_TABS,
    UI_SETTINGS_PHASE_EDIT_SETTINGS,
} UI_SETTINGS_PHASE;

// A tab for grouping settings into separate, labeled pages.
// Each tab has its own nullptr-terminated UI_SETTINGS_OPTION array.
typedef struct UI_SETTINGS_TAB {
    GAME_STRING_ID header_gs;
    const UI_SETTINGS_OPTION *options;
} UI_SETTINGS_TAB;

typedef struct {
    float row_pad;

    UI_SETTINGS_PHASE phase;
    const UI_SETTINGS_OPTION *options;
    UI_SCROLLABLE scroll;

    int32_t max_group_items;
    float max_label_w;
    float max_value_w;

    int32_t tab_count;
    const UI_SETTINGS_TAB *tabs;
    int32_t active_tab_idx;
    UI_TAB_SWITCH_STATE *tab_switch;
    GAME_STRING_ID title;

    struct {
        bool show;
        UI_TEXT_DIALOG_STATE *state;
    } description;

    int32_t listener_id;
} UI_SETTINGS_STATE;

void UI_Settings_RequestChange(const UI_SETTINGS_OPTION *option, int32_t dir);

// state functions
void UI_Settings_Init(
    UI_SETTINGS_STATE *s, GAME_STRING_ID title,
    const UI_SETTINGS_OPTION *options);
void UI_Settings_InitWithTabs(
    UI_SETTINGS_STATE *s, GAME_STRING_ID title, int32_t tab_count,
    const UI_SETTINGS_TAB *tabs);
void UI_Settings_Free(UI_SETTINGS_STATE *s);
bool UI_Settings_Control(UI_SETTINGS_STATE *s);

// draw functions
void UI_Settings(UI_SETTINGS_STATE *s);
