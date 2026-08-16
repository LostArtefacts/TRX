// The sign between the keycaps of a combination is a glyph of its own, not a
// literal plus, so a binding of several keys has to spell out as keycap, sign,
// keycap.

#include <harness/harness.h>

#include <trx/game/input/names.h>

#include <string.h>

#define M_ALT "\\{keyboard l_alt}"
#define M_RETURN "\\{keyboard return}"

static void M_CheckJoin(
    const char *const *const names, const int32_t count,
    const char *const expected)
{
    const char *const got = Input_JoinKeyNames(names, count);
    if (expected == nullptr) {
        if (got != nullptr) {
            TEST_FAIL("%d keys: got \"%s\", expected nothing", count, got);
        }
        return;
    }
    if (got == nullptr) {
        TEST_FAIL("%d keys: got nothing, expected \"%s\"", count, expected);
        return;
    }
    if (strcmp(got, expected) != 0) {
        TEST_FAIL("%d keys: got \"%s\", expected \"%s\"", count, got, expected);
    }
}

TEST(input_names_joins_a_combo_with_the_separator)
{
    const char *const names[] = { M_ALT, M_RETURN };
    M_CheckJoin(names, 2, M_ALT INPUT_COMBO_SEPARATOR M_RETURN);
}

TEST(input_names_leaves_a_single_key_alone)
{
    const char *const names[] = { M_RETURN };
    M_CheckJoin(names, 1, M_RETURN);
}

TEST(input_names_reports_nothing_for_an_unbound_role)
{
    const char *const names[] = { nullptr };
    M_CheckJoin(names, 0, nullptr);
    M_CheckJoin(names, 1, nullptr);
}

TEST(input_names_drops_a_nameless_key)
{
    const char *const leading[] = { nullptr, M_RETURN };
    M_CheckJoin(leading, 2, M_RETURN);

    const char *const trailing[] = { M_ALT, nullptr };
    M_CheckJoin(trailing, 2, M_ALT);

    const char *const middle[] = { M_ALT, nullptr, M_RETURN };
    M_CheckJoin(middle, 3, M_ALT INPUT_COMBO_SEPARATOR M_RETURN);
}
