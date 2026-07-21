#include <trx/core/completion.h>

#include <trx/core/memory.h>

#include <string.h>

void Completion_Init(COMPLETION *const c)
{
    c->start = 0;
    c->end = 0;
    c->suggestions = Vector_Create(sizeof(SUGGESTION));
}

void Completion_Clear(COMPLETION *const c)
{
    for (int32_t i = 0; i < c->suggestions->count; i++) {
        SUGGESTION *const s = Vector_Get(c->suggestions, i);
        Memory_Free(s->text);
    }
    Vector_Clear(c->suggestions);
    c->start = 0;
    c->end = 0;
}

void Completion_Free(COMPLETION *const c)
{
    Completion_Clear(c);
    Vector_Free(c->suggestions);
    c->suggestions = nullptr;
}

void Completion_Add(COMPLETION *const c, const char *const text)
{
    Completion_AddN(c, text, strlen(text));
}

void Completion_AddN(
    COMPLETION *const c, const char *const text, const size_t len)
{
    char *const dup = Memory_Alloc(len + 1);
    memcpy(dup, text, len);
    dup[len] = '\0';
    const SUGGESTION s = { .text = dup };
    Vector_Add(c->suggestions, &s);
}

char *Completion_Apply(
    const char *const line, const COMPLETION *const c,
    const char *const replacement, int32_t *const out_caret)
{
    const size_t rep_len = strlen(replacement);
    const size_t tail_len = strlen(line + c->end);
    char *const out = Memory_Alloc(c->start + rep_len + tail_len + 1);
    memcpy(out, line, c->start);
    memcpy(out + c->start, replacement, rep_len);
    memcpy(out + c->start + rep_len, line + c->end, tail_len + 1);
    if (out_caret != nullptr) {
        *out_caret = (int32_t)(c->start + rep_len);
    }
    return out;
}
