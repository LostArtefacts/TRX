#pragma once

#include <trx/core/vector.h>

#include <stddef.h>
#include <stdint.h>

// One completion candidate: the text a chosen suggestion splices into the line.
typedef struct {
    char *text;
} SUGGESTION;

// The completions for the token under the caret: the byte span [start, end) of
// the line a chosen suggestion replaces, and the candidates for it, best first.
// The span can reach past the caret - to the end of the word, or the end of the
// line for a greedy argument. `suggestions` owns each SUGGESTION's text.
typedef struct {
    size_t start;
    size_t end;
    VECTOR *suggestions;
} COMPLETION;

// A completion source: `fn` fills `out` for the token under the caret in
// `line`, with an absolute [start, end) span; `ctx` carries what the source
// needs to resolve it, or nullptr. A zeroed COMPLETER (fn == nullptr) completes
// nothing.
typedef struct {
    void (*fn)(void *ctx, const char *line, int32_t caret, COMPLETION *out);
    void *ctx;
} COMPLETER;

// Picks the completer that applies at `caret` in `line`.
typedef COMPLETER (*COMPLETER_PROVIDER)(const char *line, int32_t caret);

// A COMPLETION begins empty, with a span of [0, 0). Init once; Clear between
// queries to reuse it; Free to release it, including the suggestions vector.
void Completion_Init(COMPLETION *c);
void Completion_Clear(COMPLETION *c);
void Completion_Free(COMPLETION *c);

// Appends a suggestion, copying `text`.
void Completion_Add(COMPLETION *c, const char *text);

// Appends a suggestion, copying `len` bytes of `text`, for a source that is not
// nul-terminated at the point it ends.
void Completion_AddN(COMPLETION *c, const char *text, size_t len);

// Splices `replacement` over the completion's [start, end) span in `line`,
// returning the resulting line (heap-allocated, caller frees) and, in
// `*out_caret`, the caret at the end of what was spliced in. The bytes before
// the span and the tail after it are kept, so a candidate lands over exactly
// the run the span marks out.
char *Completion_Apply(
    const char *line, const COMPLETION *c, const char *replacement,
    int32_t *out_caret);
