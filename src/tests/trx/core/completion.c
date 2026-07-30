#include <harness/harness.h>

#include <trx/core/completion.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>

#include <string.h>

// Completion_Apply is the console-agnostic splice: a line, a span and a
// replacement in, a new line and caret out.

TEST(apply_replaces_the_span_and_keeps_the_tail)
{
    COMPLETION c;
    Completion_Init(&c);
    c.start = 5;
    c.end = 14;
    int32_t caret = 0;
    char *const line =
        Completion_Apply("play angkor wa", &c, "Angkor Wat", &caret);
    CHECK_EQ_STR(line, "play Angkor Wat");
    CHECK_EQ_INT(caret, 15);
    Memory_Free(line);
    Completion_Free(&c);
}

TEST(apply_keeps_text_after_the_span)
{
    COMPLETION c;
    Completion_Init(&c);
    c.start = 4;
    c.end = 10;
    int32_t caret = 0;
    char *const line = Completion_Apply("set option value", &c, "fov", &caret);
    CHECK_EQ_STR(line, "set fov value");
    CHECK_EQ_INT(caret, 7);
    Memory_Free(line);
    Completion_Free(&c);
}

TEST(apply_inserts_when_the_span_is_empty)
{
    COMPLETION c;
    Completion_Init(&c);
    c.start = 5;
    c.end = 5;
    int32_t caret = 0;
    char *const line = Completion_Apply("play ", &c, "Caves", &caret);
    CHECK_EQ_STR(line, "play Caves");
    CHECK_EQ_INT(caret, 10);
    Memory_Free(line);
    Completion_Free(&c);
}

TEST(add_copies_the_text)
{
    COMPLETION c;
    Completion_Init(&c);
    char buf[] = "temp";
    Completion_Add(&c, buf);
    strcpy(buf, "gone");
    const SUGGESTION *const s = Vector_Get(c.suggestions, 0);
    CHECK_EQ_STR(s->text, "temp");
    Completion_Free(&c);
}

TEST(clear_empties_but_keeps_the_vector)
{
    COMPLETION c;
    Completion_Init(&c);
    Completion_Add(&c, "a");
    c.start = 2;
    c.end = 5;
    Completion_Clear(&c);
    CHECK_EQ_INT(c.suggestions->count, 0);
    CHECK_EQ_INT(c.start, 0);
    CHECK_EQ_INT(c.end, 0);
    CHECK_NOT_NULL(c.suggestions);
    Completion_Free(&c);
}
