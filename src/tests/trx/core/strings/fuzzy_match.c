#include <harness/harness.h>

#include <trx/core/memory.h>
#include <trx/core/strings/fuzzy_match.h>
#include <trx/core/vector.h>

// String_FuzzyMatch ranks a list of keys against what the player typed. The
// input reaches a regular expression, so a key holding punctuation is the
// interesting case.

static VECTOR *M_Sources(const char *const *const keys, const int32_t count)
{
    VECTOR *const sources = Vector_Create(sizeof(STRING_FUZZY_SOURCE));
    for (int32_t i = 0; i < count; i++) {
        const STRING_FUZZY_SOURCE source = {
            .key = keys[i],
            .value = (void *)keys[i],
            .weight = 1,
        };
        Vector_Add(sources, &source);
    }
    return sources;
}

TEST(matches_a_plain_name)
{
    const char *const keys[] = { "Coastal Village", "Jungle" };
    VECTOR *const sources = M_Sources(keys, 2);
    VECTOR *const matches = String_FuzzyMatch("Jungle", sources);
    CHECK_EQ_INT(matches->count, 1);
    CHECK_EQ_STR(((STRING_FUZZY_MATCH *)Vector_Get(matches, 0))->key, "Jungle");
    Vector_Free(matches);
    Vector_Free(sources);
}

TEST(reads_punctuation_as_itself)
{
    const char *const keys[] = { "Lud's Gate", "Area 51" };
    VECTOR *const sources = M_Sources(keys, 2);
    // A dot stands for any character in a regular expression. Read as itself,
    // it belongs to neither key.
    VECTOR *const matches = String_FuzzyMatch("Lud.s Gate", sources);
    CHECK_EQ_INT(matches->count, 0);
    Vector_Free(matches);
    Vector_Free(sources);
}

TEST(takes_input_that_would_not_compile)
{
    const char *const keys[] = { "Coastal Village", "Jungle" };
    VECTOR *const sources = M_Sources(keys, 2);
    VECTOR *const matches = String_FuzzyMatch("[", sources);
    CHECK_EQ_INT(matches->count, 0);
    Vector_Free(matches);
    Vector_Free(sources);
}

TEST(finds_a_key_that_holds_a_metacharacter)
{
    const char *const keys[] = { "C++ notes", "C sharp" };
    VECTOR *const sources = M_Sources(keys, 2);
    VECTOR *const matches = String_FuzzyMatch("C++", sources);
    CHECK_EQ_INT(matches->count, 1);
    CHECK_EQ_STR(
        ((STRING_FUZZY_MATCH *)Vector_Get(matches, 0))->key, "C++ notes");
    Vector_Free(matches);
    Vector_Free(sources);
}
