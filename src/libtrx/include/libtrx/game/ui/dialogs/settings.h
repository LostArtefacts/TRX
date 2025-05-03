#pragma once

#include "../../../config/option.h"
#include "../../../config/types.h"
#include "../../game_string.h"
#include "../common.h"
#include "../elements/requester.h"

typedef struct {
    int32_t value;
    GAME_STRING_ID name;
} UI_SETTINGS_ENUM_ENTRY;

typedef struct {
    CONFIG_OPTION_TYPE option_type;
    GAME_STRING_ID label_id;
    void *target;
    int32_t min_value;
    int32_t max_value;
    int32_t delta_slow;
    int32_t delta_fast;
    const void *misc;
} UI_SETTINGS_OPTION;

typedef struct {
    const UI_SETTINGS_OPTION *options;
    UI_REQUESTER_STATE req;
    float arrow_spacing;
    float value_w;
} UI_SETTINGS_STATE;

void UI_Settings_Init(UI_SETTINGS_STATE *s, const UI_SETTINGS_OPTION *options);
void UI_Settings_Free(UI_SETTINGS_STATE *s);
bool UI_Settings_Control(UI_SETTINGS_STATE *s);

void UI_Settings(UI_SETTINGS_STATE *s);
