#include "game/ui/scrollable.h"

#include "config.h"
#include "utils.h"

bool UI_Scrollable_SelectNext(
    UI_SCROLLABLE *const s, const bool enable_wraparound)
{
    if (s->sel_item + 1 < s->max_items) {
        s->sel_item++;
    } else if (enable_wraparound) {
        s->sel_item = 0;
    } else {
        return false;
    }
    CLAMP(s->first_item, s->sel_item - s->vis_items + 1, s->sel_item);
    CLAMPG(s->first_item, s->max_items - s->vis_items);
    CLAMPL(s->first_item, 0);
    return true;
}

bool UI_Scrollable_SelectPrev(
    UI_SCROLLABLE *const s, const bool enable_wraparound)
{
    if (s->sel_item > 0) {
        s->sel_item--;
    } else if (enable_wraparound) {
        s->sel_item = s->max_items - 1;
    } else {
        return false;
    }
    CLAMP(s->first_item, s->sel_item - s->vis_items + 1, s->sel_item);
    CLAMPG(s->first_item, s->max_items - s->vis_items);
    CLAMPL(s->first_item, 0);
    return true;
}

bool UI_Scrollable_ScrollDown(
    UI_SCROLLABLE *const s, const bool enable_wraparound)
{
    if (s->first_item + 1 <= s->max_items - s->vis_items) {
        s->first_item++;
    } else if (enable_wraparound) {
        s->first_item = 0;
    } else {
        return false;
    }
    CLAMPG(s->first_item, s->max_items - s->vis_items);
    CLAMPL(s->first_item, 0);
    return true;
}

bool UI_Scrollable_ScrollUp(
    UI_SCROLLABLE *const s, const bool enable_wraparound)
{
    if (s->first_item > 0) {
        s->first_item--;
    } else if (enable_wraparound) {
        s->first_item = s->max_items - 1;
    } else {
        return false;
    }
    CLAMPG(s->first_item, s->max_items - s->vis_items);
    CLAMPL(s->first_item, 0);
    return true;
}

void UI_Scrollable_SetVisibleItems(
    UI_SCROLLABLE *const s, const int32_t visible_rows)
{
    s->vis_items = visible_rows;
    CLAMPL(s->vis_items, 0);
    if (s->sel_item != -1) {
        CLAMP(s->first_item, s->sel_item - s->vis_items + 1, s->sel_item);
    }
    CLAMP(s->first_item, 0, s->max_items - s->vis_items);
}

void UI_Scrollable_SetMaxItems(UI_SCROLLABLE *const s, const int32_t max_items)
{
    s->max_items = max_items;
    CLAMP(s->sel_item, 0, s->max_items - 1);
}

void UI_Scrollable_SelectItem(UI_SCROLLABLE *const s, const int32_t row)
{
    s->sel_item = row;
    if (s->sel_item != -1) {
        CLAMP(s->sel_item, 0, s->max_items - 1);
        CLAMP(s->first_item, s->sel_item - s->vis_items + 1, s->sel_item);
    }
}

void UI_Scrollable_SelectFirstItem(UI_SCROLLABLE *const s)
{
    UI_Scrollable_SelectItem(s, 0);
}

void UI_Scrollable_SelectLastItem(UI_SCROLLABLE *const s)
{
    UI_Scrollable_SelectItem(s, s->max_items - 1);
}

int32_t UI_Scrollable_GetFirstVisibleItem(const UI_SCROLLABLE *const s)
{
    return s->first_item;
}

int32_t UI_Scrollable_GetSelectedItem(const UI_SCROLLABLE *const s)
{
    return s->sel_item;
}

int32_t UI_Scrollable_GetLastVisibleItem(const UI_SCROLLABLE *const s)
{
    return MIN(s->first_item + s->vis_items - 1, s->max_items - 1);
}
