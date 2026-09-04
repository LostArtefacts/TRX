#include <harness/harness.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>

// String_ToFileName rewrites text so that it can name a file. The buffer it
// fills is as long as the text it reads, so text that survives whole is the
// case that reaches the end of it.

static void M_CheckFileName(const char *const input, const char *const expected)
{
    char *const result = String_ToFileName(input);
    CHECK_EQ_STR(result, expected);
    Memory_Free(result);
}

TEST(file_name_fills_the_buffer_it_allocates)
{
    // Fifteen characters in and fifteen out, which is one short of the buffer.
    M_CheckFileName("Coastal Village", "Coastal_Village");
}

TEST(file_name_keeps_text_that_needs_nothing_done)
{
    M_CheckFileName("Jungle", "Jungle");
}

TEST(file_name_turns_spaces_into_underscores)
{
    M_CheckFileName("Temple Ruins", "Temple_Ruins");
}

TEST(file_name_merges_runs_of_underscores)
{
    M_CheckFileName("Area   51", "Area_51");
    M_CheckFileName("Area _ 51", "Area_51");
}

TEST(file_name_trims_underscores_from_both_ends)
{
    M_CheckFileName("  Trailing  ", "Trailing");
    M_CheckFileName("___", "");
}

TEST(file_name_drops_reserved_characters)
{
    M_CheckFileName("a/b\\c:d*e?f\"g<h>i|j", "abcdefghij");
}

TEST(file_name_keeps_text_outside_ascii)
{
    M_CheckFileName("\xc3\xa9t\xc3\xa9 Village", "\xc3\xa9t\xc3\xa9_Village");
}

TEST(file_name_takes_text_ending_mid_sequence)
{
    // A lone lead byte claims more bytes than the text holds.
    M_CheckFileName("Bad\xc3", "Bad\xc3");
}

TEST(file_name_takes_empty_text)
{
    M_CheckFileName("", "");
}

static void M_CheckWrap(
    const char *const input, const int32_t columns, const char *const expected)
{
    char *const result = String_Wrap(input, columns);
    CHECK_EQ_STR(result, expected);
    Memory_Free(result);
}

TEST(wrap_leaves_text_that_fits)
{
    M_CheckWrap("Lara has the shotgun", 40, "Lara has the shotgun");
}

TEST(wrap_breaks_between_words)
{
    M_CheckWrap("Lara has the shotgun", 12, "Lara has the\nshotgun");
}

TEST(wrap_keeps_a_break_that_is_there)
{
    M_CheckWrap("Lara\nhas the shotgun", 40, "Lara\nhas the shotgun");
}

TEST(wrap_breaks_a_word_longer_than_a_line)
{
    M_CheckWrap("/a/very/long/path", 8, "/a/very/\nlong/pat\nh");
}

TEST(wrap_takes_empty_text)
{
    M_CheckWrap("", 40, "");
}
