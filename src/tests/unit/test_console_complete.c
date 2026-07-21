#include "harness.h"

#include <trx/core/completion.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/game/console/completion.h>
#include <trx/game/console/registry.h>

#include <string.h>

static COMMAND_RESULT M_Dummy(const COMMAND_CONTEXT *const ctx)
{
    return CR_SUCCESS;
}

// A stub argument completer reports a fixed region-relative span, so a case can
// watch where the selector shifts it onto the line.
static size_t m_StubStart;
static size_t m_StubEnd;

static void M_StubComplete(
    const struct CONSOLE_COMMAND *const cmd, const char *const text,
    const int32_t caret, COMPLETION *const out)
{
    out->start = m_StubStart;
    out->end = m_StubEnd;
    Completion_Add(out, "candidate");
}

static void M_Register(
    const char *const name, const char *const aliases,
    void (*const complete)(
        const struct CONSOLE_COMMAND *, const char *, int32_t, COMPLETION *))
{
    Console_Registry_Add((CONSOLE_COMMAND) {
        .prefix = name,
        .proc = M_Dummy,
        .complete = complete,
        .aliases = aliases,
    });
}

// Runs the completer the selector picks for `line` at `caret`, filling `out`.
static void M_Complete(
    const char *const line, const int32_t caret, COMPLETION *const out)
{
    const COMPLETER e = Console_GetCompleter(line, caret);
    if (e.fn != nullptr) {
        e.fn(e.ctx, line, caret, out);
    }
}

static bool M_Has(const COMPLETION *const c, const char *const s)
{
    for (int32_t i = 0; i < c->suggestions->count; i++) {
        const SUGGESTION *const sg = Vector_Get(c->suggestions, i);
        if (strcmp(sg->text, s) == 0) {
            return true;
        }
    }
    return false;
}

TEST(the_command_word_completes_from_the_registry)
{
    M_Register("set", nullptr, nullptr);
    M_Register("sfx", nullptr, nullptr);

    COMPLETION c;
    Completion_Init(&c);
    M_Complete("s", 1, &c);
    CHECK_EQ_INT(c.start, 0);
    CHECK_EQ_INT(c.end, 1);
    CHECK(M_Has(&c, "set"));
    CHECK(M_Has(&c, "sfx"));
    Completion_Free(&c);
}

TEST(the_command_word_suggests_aliases_too)
{
    M_Register("give", "keys, guns", nullptr);

    COMPLETION c;
    Completion_Init(&c);
    M_Complete("k", 1, &c);
    CHECK(M_Has(&c, "keys"));
    Completion_Free(&c);
}

// Completing with the caret inside the command word spans the whole word, so a
// candidate lands over all of it rather than leaving the tail past the caret.
TEST(the_command_word_span_covers_the_whole_word)
{
    M_Register("set", nullptr, nullptr);

    COMPLETION c;
    Completion_Init(&c);
    M_Complete("set", 1, &c);
    CHECK_EQ_INT(c.start, 0);
    CHECK_EQ_INT(c.end, 3);
    Completion_Free(&c);
}

// A caret in the whitespace before the command word spans the word, not the
// indent, so a candidate replaces the word and leaves the leading spaces
// intact.
TEST(a_caret_in_the_leading_whitespace_is_safe)
{
    M_Register("set", nullptr, nullptr);

    COMPLETION c;
    Completion_Init(&c);
    M_Complete("  set", 1, &c);
    CHECK_EQ_INT(c.start, 2);
    CHECK_EQ_INT(c.end, 5);
    CHECK(M_Has(&c, "set"));
    Completion_Free(&c);
}

// The command's region-relative span is shifted onto the line by the command
// word and the gap after it.
TEST(an_argument_span_is_shifted_onto_the_line)
{
    M_Register("sfx", nullptr, M_StubComplete);
    // "sfx fov 9": the region after "sfx" is " fov 9"; a span over its last
    // token, region bytes [5, 6), lands at 3 + [5, 6) on the line.
    m_StubStart = 5;
    m_StubEnd = 6;

    COMPLETION c;
    Completion_Init(&c);
    M_Complete("sfx fov 9", 9, &c);
    CHECK_EQ_INT(c.start, 8);
    CHECK_EQ_INT(c.end, 9);
    CHECK(M_Has(&c, "candidate"));
    Completion_Free(&c);
}

// A greedy span reaches the region end; shifted, it reaches the line end even
// with the caret mid-tail.
TEST(a_greedy_argument_span_reaches_the_line_end)
{
    M_Register("play", nullptr, M_StubComplete);
    // "play angkor wa": the region " angkor wa" is spanned whole, bytes
    // [1, 10); shifted by 4 that is [5, 14), to the line end.
    m_StubStart = 1;
    m_StubEnd = 10;

    COMPLETION c;
    Completion_Init(&c);
    M_Complete("play angkor wa", 10, &c);
    CHECK_EQ_INT(c.start, 5);
    CHECK_EQ_INT(c.end, 14);
    Completion_Free(&c);
}

// A caret past the end of the line is clamped, not read past.
TEST(a_caret_past_the_end_is_clamped)
{
    M_Register("set", nullptr, nullptr);

    COMPLETION c;
    Completion_Init(&c);
    M_Complete("set", 99, &c);
    CHECK_EQ_INT(c.start, 0);
    CHECK_EQ_INT(c.end, 3);
    Completion_Free(&c);
}
