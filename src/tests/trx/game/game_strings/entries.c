#include <harness/harness.h>

#include <trx/game/game_strings/entries.h>

// Cover direct strings, object references, parent references and reference
// loops.

TEST(a_string_is_the_words_it_holds)
{
    GameString_Clear();
    GameString_Define("objects/lead_bar_item/name", "Lead Bar");
    CHECK_EQ_STR(GameString_Get("objects/lead_bar_item/name"), "Lead Bar");
    GameString_Clear();
}

TEST(a_path_is_read_at_the_object_its_parent_names)
{
    GameString_Clear();
    GameString_Define("objects/lead_bar_item/name", "Lead Bar");
    GameString_Define("objects/lead_bar_item/description", "A bar of lead.");
    GameString_Define("objects/lead_bar_option", "$objects/lead_bar_item");
    CHECK_EQ_STR(GameString_Get("objects/lead_bar_option/name"), "Lead Bar");
    CHECK_EQ_STR(
        GameString_Get("objects/lead_bar_option/description"),
        "A bar of lead.");
    GameString_Clear();
}

TEST(a_reference_takes_the_words_last_laid_over_it)
{
    GameString_Clear();
    GameString_Define("objects/key_item_1/name", "Key 1");
    GameString_Define("objects/key_option_1", "$objects/key_item_1");
    GameString_Define("objects/key_item_1/name", "Silver Key");
    CHECK_EQ_STR(GameString_Get("objects/key_option_1/name"), "Silver Key");
    GameString_Clear();
}

TEST(words_of_its_own_come_before_the_ones_a_parent_names)
{
    GameString_Clear();
    GameString_Define("objects/flare_item/name", "Flare");
    GameString_Define("objects/flares_box_option", "$objects/flare_item");
    GameString_Define("objects/flares_box_option/name", "Box of Flares");
    CHECK_EQ_STR(
        GameString_Get("objects/flares_box_option/name"), "Box of Flares");
    GameString_Clear();
}

TEST(a_chain_of_references_runs_to_its_end)
{
    GameString_Clear();
    GameString_Define("objects/flare_item/name", "Flare");
    GameString_Define("objects/flare_option", "$objects/flare_item");
    GameString_Define("objects/flares_box_option", "$objects/flare_option");
    CHECK_EQ_STR(GameString_Get("objects/flares_box_option/name"), "Flare");
    GameString_Clear();
}

TEST(a_reference_that_names_nothing_holds_no_words)
{
    GameString_Clear();
    GameString_Define("objects/lead_bar_option", "$objects/lead_bar_item");
    CHECK_NULL(GameString_Get("objects/lead_bar_option/description"));
    GameString_Clear();
}

TEST(a_loop_of_references_holds_no_words)
{
    GameString_Clear();
    GameString_Define("objects/a", "$objects/b");
    GameString_Define("objects/b", "$objects/a");
    CHECK_NULL(GameString_Get("objects/a/name"));
    GameString_Clear();
}
