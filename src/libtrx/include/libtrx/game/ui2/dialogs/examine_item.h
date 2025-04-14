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
} UI2_EXAMINE_ITEM_STATE;

void UI2_ExamineItem_Init(
    UI2_EXAMINE_ITEM_STATE *state, const char *title, const char *text,
    size_t max_lines);
void UI2_ExamineItem_Control(UI2_EXAMINE_ITEM_STATE *state);
void UI2_ExamineItem_Free(UI2_EXAMINE_ITEM_STATE *state);

void UI2_ExamineItem(UI2_EXAMINE_ITEM_STATE *state);
