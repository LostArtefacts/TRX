#include <trx/game/console/completion.h>

#include <trx/core/memory.h>
#include <trx/game/console/registry.h>

#include <string.h>

// The command word is the leading run of non-space characters after any indent.
static void M_WordBounds(
    const char *const line, size_t *const start, size_t *const end)
{
    size_t s = 0;
    while (line[s] == ' ') {
        s++;
    }
    size_t e = s;
    while (line[e] != '\0' && line[e] != ' ') {
        e++;
    }
    *start = s;
    *end = e;
}

static size_t M_ClampCaret(const char *const line, const int32_t caret)
{
    if (caret < 0) {
        return 0;
    }
    const size_t line_len = strlen(line);
    return (size_t)caret > line_len ? line_len : (size_t)caret;
}

// The command word, or the whitespace before it: complete command names. The
// span is the word itself, so a candidate replaces it and leaves any indent
// untouched. The prefix is the word up to the caret, empty when the caret sits
// in the whitespace before it.
static void M_CompleteNames(
    void *const ctx, const char *const line, const int32_t caret_i,
    COMPLETION *const out)
{
    const size_t caret = M_ClampCaret(line, caret_i);
    size_t word_start, word_end;
    M_WordBounds(line, &word_start, &word_end);
    out->start = word_start;
    out->end = word_end;
    const size_t prefix_len =
        (caret < word_start ? word_start : caret) - word_start;
    char *prefix = Memory_Alloc(prefix_len + 1);
    memcpy(prefix, line + word_start, prefix_len);
    prefix[prefix_len] = '\0';
    Console_Registry_Suggest(prefix, out);
    Memory_FreePointer(&prefix);
}

// A command's arguments: the command's completer works over the region after
// the command word (keeping the gap, so a caret there stays inside it) and
// reports a region-relative span; shift it onto the line.
static void M_CompleteArgs(
    void *const ctx, const char *const line, const int32_t caret_i,
    COMPLETION *const out)
{
    const CONSOLE_COMMAND *const cmd = ctx;
    const size_t caret = M_ClampCaret(line, caret_i);
    size_t word_start, word_end;
    M_WordBounds(line, &word_start, &word_end);
    const size_t base = word_end;
    out->start = caret - base;
    out->end = caret - base;
    cmd->complete(cmd, line + base, (int32_t)(caret - base), out);
    out->start += base;
    out->end += base;
}

COMPLETER Console_GetCompleter(const char *const line, const int32_t caret_i)
{
    const size_t caret = M_ClampCaret(line, caret_i);
    size_t word_start, word_end;
    M_WordBounds(line, &word_start, &word_end);

    if (caret <= word_end) {
        return (COMPLETER) { .fn = M_CompleteNames };
    }

    const CONSOLE_COMMAND *const cmd = Console_Registry_Get(line + word_start);
    if (cmd == nullptr || cmd->complete == nullptr) {
        return (COMPLETER) { 0 };
    }
    return (COMPLETER) { .fn = M_CompleteArgs, .ctx = (void *)cmd };
}
