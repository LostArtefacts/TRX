#pragma once

#include "../../../config/option.h"
#include "../../../config/types.h"
#include "../../../utils.h"
#include "../../game_string.h"
#include "../common.h"
#include "../elements/tab_switch.h"
#include "../scrollable.h"
#include "./text.h"

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

#define X_UI_CFG_MANUAL(label_id_, ...)                                        \
    { .label_id = GS_ID(label_id_),                                            \
      .description_id = GS_ID(label_id_##_DESCRIPTION),                        \
      __VA_ARGS__ },

#define X_UI_CFG(opt_type_, target_, label_id_, ...)                           \
    X_UI_CFG_MANUAL(                                                           \
        label_id_, .target = &g_Config.target_, .option_type = opt_type_,      \
        __VA_ARGS__)

#define X_UI_CFG_STRING(target_, label_id_, ...)                               \
    X_UI_CFG(COT_STRING, target_, label_id_, ##__VA_ARGS__)

#define X_UI_CFG_BOOL(target_, label_id_, ...)                                 \
    X_UI_CFG(COT_BOOL, target_, label_id_, ##__VA_ARGS__)

#define X_UI_CFG_BOOL_INV(target_, label_id_, ...)                             \
    X_UI_CFG(COT_INVERTED_BOOL, target_, label_id_, ##__VA_ARGS__)

#define X_UI_CFG_INT32(target_, label_id_, ...)                                \
    X_UI_CFG(COT_INT32, target_, label_id_, ##__VA_ARGS__)

#define X_UI_CFG_FLOAT(target_, label_id_, ...)                                \
    X_UI_CFG(COT_FLOAT, target_, label_id_, ##__VA_ARGS__)

#define X_UI_CFG_FLOAT_PERCENT(target_, label_id_, ...)                        \
    X_UI_CFG(COT_FLOAT_PERCENT, target_, label_id_, ##__VA_ARGS__)

#define X_UI_CFG_DOUBLE(target_, label_id_, ...)                               \
    X_UI_CFG(COT_DOUBLE, target_, label_id_, ##__VA_ARGS__)

#define X_UI_CFG_ENUM(target_, label_id_, ...)                                 \
    X_UI_CFG(                                                                  \
        COT_ENUM, target_, label_id_, .delta_slow = 1, .delta_fast = 1,        \
        ##__VA_ARGS__)

#define X_UI_CFG_RGB888(target_, label_id_, component, ...)                    \
    X_UI_CFG(                                                                  \
        COT_RGB888, target_, label_id_, .delta_slow = 1, .delta_fast = 10,     \
        .min_value = 0, .max_value = 255, .misc = (void *)(intptr_t)component, \
        ##__VA_ARGS__)

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

typedef struct UI_SETTINGS_STATE UI_SETTINGS_STATE;

void UI_Settings_RequestChange(const UI_SETTINGS_OPTION *option, int32_t dir);

// state functions
UI_SETTINGS_STATE *UI_Settings_Init(
    GAME_STRING_ID title, const UI_SETTINGS_OPTION *options);
UI_SETTINGS_STATE *UI_Settings_InitWithTabs(
    GAME_STRING_ID title, int32_t tab_count, const UI_SETTINGS_TAB *tabs);
void UI_Settings_Free(UI_SETTINGS_STATE *s);
bool UI_Settings_Control(UI_SETTINGS_STATE *s);

// draw functions
void UI_Settings(UI_SETTINGS_STATE *s);
