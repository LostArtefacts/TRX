#include <harness/harness.h>

#include <trx/game/ui/scrollable.h>

static UI_SCROLLABLE M_Make(
    const int32_t max_items, const int32_t vis_items, const int32_t first_item,
    const int32_t sel_item)
{
    return (UI_SCROLLABLE) {
        .first_item = first_item,
        .sel_item = sel_item,
        .max_items = max_items,
        .vis_items = vis_items,
    };
}

TEST(moving_down_past_the_last_visible_item_scrolls)
{
    UI_SCROLLABLE s = M_Make(10, 4, 0, 3);
    CHECK(UI_Scrollable_SelectNext(&s, false));
    CHECK_EQ_INT(s.sel_item, 4);
    CHECK_EQ_INT(s.first_item, 1);
}

TEST(moving_down_off_the_end_stays_put_without_wraparound)
{
    UI_SCROLLABLE s = M_Make(10, 4, 6, 9);
    CHECK(!UI_Scrollable_SelectNext(&s, false));
    CHECK_EQ_INT(s.sel_item, 9);
    CHECK_EQ_INT(s.first_item, 6);
}

TEST(wrapping_down_scrolls_back_to_the_top)
{
    UI_SCROLLABLE s = M_Make(10, 4, 6, 9);
    CHECK(UI_Scrollable_SelectNext(&s, true));
    CHECK_EQ_INT(s.sel_item, 0);
    CHECK_EQ_INT(s.first_item, 0);
}

TEST(wrapping_up_scrolls_to_the_bottom)
{
    UI_SCROLLABLE s = M_Make(10, 4, 0, 0);
    CHECK(UI_Scrollable_SelectPrev(&s, true));
    CHECK_EQ_INT(s.sel_item, 9);
    CHECK_EQ_INT(s.first_item, 6);
}

TEST(a_shorter_list_pulls_the_view_back_to_its_end)
{
    // The controls editor keeps one scroll state across its tabs. Switching
    // from a long tab, scrolled to its end, to a shorter one used to leave the
    // view scrolled past the new end, and the next keypress snapped it back.
    UI_SCROLLABLE s = M_Make(19, 13, 6, 18);
    UI_Scrollable_SetMaxItems(&s, 15);
    CHECK_EQ_INT(s.sel_item, 14);
    CHECK_EQ_INT(s.first_item, 2);
    CHECK(UI_Scrollable_IsItemVisible(&s, s.sel_item));

    CHECK(UI_Scrollable_SelectPrev(&s, false));
    CHECK_EQ_INT(s.first_item, 2);
}

TEST(a_list_shorter_than_the_view_starts_at_its_first_item)
{
    UI_SCROLLABLE s = M_Make(19, 13, 6, 18);
    UI_Scrollable_SetMaxItems(&s, 5);
    CHECK_EQ_INT(s.sel_item, 4);
    CHECK_EQ_INT(s.first_item, 0);
}

TEST(a_longer_list_leaves_the_view_where_it_was)
{
    UI_SCROLLABLE s = M_Make(15, 13, 2, 14);
    UI_Scrollable_SetMaxItems(&s, 19);
    CHECK_EQ_INT(s.sel_item, 14);
    CHECK_EQ_INT(s.first_item, 2);
}

TEST(a_taller_view_shows_more_of_the_list)
{
    UI_SCROLLABLE s = M_Make(10, 4, 6, 9);
    UI_Scrollable_SetVisibleItems(&s, 8);
    CHECK_EQ_INT(s.first_item, 2);
    CHECK_EQ_INT(UI_Scrollable_GetLastVisibleItem(&s), 9);
}

TEST(selecting_the_last_item_scrolls_it_into_view)
{
    UI_SCROLLABLE s = M_Make(10, 4, 0, 0);
    UI_Scrollable_SelectLastItem(&s);
    CHECK_EQ_INT(s.sel_item, 9);
    CHECK(UI_Scrollable_IsItemVisible(&s, 9));
    CHECK(!UI_Scrollable_IsItemVisible(&s, 5));
    CHECK(UI_Scrollable_IsItemSelected(&s, 9));
}
