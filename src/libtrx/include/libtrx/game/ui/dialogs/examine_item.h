#pragma once

#include "../../../vector.h"
#include "../common.h"

// A widget to cycle through several pages of a text content.

typedef struct {
    char *title;
    size_t max_lines;
    int32_t current_page;
    VECTOR *page_content;
    bool is_empty;
} UI_EXAMINE_ITEM_STATE;

void UI_ExamineItem_Init(
    UI_EXAMINE_ITEM_STATE *state, const char *title, const char *text,
    size_t max_lines);
void UI_ExamineItem_Control(UI_EXAMINE_ITEM_STATE *state);
void UI_ExamineItem_Free(UI_EXAMINE_ITEM_STATE *state);

void UI_ExamineItem(UI_EXAMINE_ITEM_STATE *state);
