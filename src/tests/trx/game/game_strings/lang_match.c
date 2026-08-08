#include <harness/harness.h>

#include <trx/core/vector.h>
#include <trx/game/game_strings/lang_match.h>

// The codes on either side are the ones the strings files are discovered
// under - "en", "en-gb", "pl" - and the ones the player's system reports.

static VECTOR *M_Codes(const char *const *const codes, const int32_t count)
{
    VECTOR *const out = Vector_Create(sizeof(char *));
    for (int32_t i = 0; i < count; i++) {
        Vector_Add(out, &codes[i]);
    }
    return out;
}

#define CODES(...)                                                             \
    M_Codes(                                                                   \
        (const char *[]) { __VA_ARGS__ },                                      \
        sizeof((const char *[]) { __VA_ARGS__ }) / sizeof(const char *))

TEST(the_wanted_language_is_taken_where_it_ships)
{
    VECTOR *const available = CODES("en", "de", "pl");
    VECTOR *const preferred = CODES("pl");
    CHECK_EQ_STR(GameStringLang_MatchPreferred(available, preferred), "pl");
    Vector_Free(available);
    Vector_Free(preferred);
}

TEST(nothing_is_taken_where_no_wanted_language_ships)
{
    VECTOR *const available = CODES("en", "de");
    VECTOR *const preferred = CODES("ja", "ko");
    CHECK_NULL(GameStringLang_MatchPreferred(available, preferred));
    Vector_Free(available);
    Vector_Free(preferred);
}

TEST(the_second_choice_is_taken_where_the_first_does_not_ship)
{
    VECTOR *const available = CODES("en", "de");
    VECTOR *const preferred = CODES("ja", "de");
    CHECK_EQ_STR(GameStringLang_MatchPreferred(available, preferred), "de");
    Vector_Free(available);
    Vector_Free(preferred);
}

TEST(a_region_is_honoured_before_it_is_discarded)
{
    VECTOR *const available = CODES("en", "en-gb");
    VECTOR *const preferred = CODES("en-gb");
    CHECK_EQ_STR(GameStringLang_MatchPreferred(available, preferred), "en-gb");
    Vector_Free(available);
    Vector_Free(preferred);
}

TEST(the_language_alone_answers_a_region_that_does_not_ship)
{
    VECTOR *const available = CODES("en", "en-gb");
    VECTOR *const preferred = CODES("en-au");
    CHECK_EQ_STR(GameStringLang_MatchPreferred(available, preferred), "en");
    Vector_Free(available);
    Vector_Free(preferred);
}

TEST(another_region_answers_last)
{
    VECTOR *const available = CODES("en", "pt-br");
    VECTOR *const preferred = CODES("pt-pt");
    CHECK_EQ_STR(GameStringLang_MatchPreferred(available, preferred), "pt-br");
    Vector_Free(available);
    Vector_Free(preferred);
}

TEST(an_exact_later_choice_beats_a_region_match_on_an_earlier_one)
{
    VECTOR *const available = CODES("pt-br", "de");
    VECTOR *const preferred = CODES("pt-pt", "de");
    CHECK_EQ_STR(GameStringLang_MatchPreferred(available, preferred), "de");
    Vector_Free(available);
    Vector_Free(preferred);
}

TEST(case_does_not_matter)
{
    VECTOR *const available = CODES("en", "pt-br");
    VECTOR *const preferred = CODES("PT-BR");
    CHECK_EQ_STR(GameStringLang_MatchPreferred(available, preferred), "pt-br");
    Vector_Free(available);
    Vector_Free(preferred);
}

TEST(a_system_that_says_nothing_leaves_the_choice_alone)
{
    VECTOR *const available = CODES("en", "de");
    VECTOR *const preferred = Vector_Create(sizeof(char *));
    CHECK_NULL(GameStringLang_MatchPreferred(available, preferred));
    Vector_Free(available);
    Vector_Free(preferred);
}
