// core/result.c reaches outside core for its message formatting and for the
// fatal exit. Both live in translation units that drag in the engine's
// dependencies - strings/common.c wants PCRE2, and the shell wants SDL - and
// neither is what a module handing back a RESULT is being tested for.
//
// Standing them up here keeps the unit tests engine-free, which is the whole
// reason they are a separate project (see meson.build).

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Weak, so a test that links core/strings or fakes the shell itself gets that
// instead of this.
__attribute__((weak)) char *String_Format(const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int size = vsnprintf(nullptr, 0, fmt, args) + 1;
    va_end(args);

    // malloc, not Memory_Alloc: a test with no core/memory.c in its closure
    // still links this file. Memory_Free is plain free, so a caller can hand
    // the string back either way.
    char *const result = malloc(size);
    va_start(args, fmt);
    vsnprintf(result, size, fmt, args);
    va_end(args);
    return result;
}

__attribute__((weak)) const char *String_FormatStaticV(
    const char *const fmt, va_list args)
{
    static char result[1024];
    vsnprintf(result, sizeof(result), fmt, args);
    return result;
}

__attribute__((weak)) void Shell_ExitSystem(const char *const message)
{
    fprintf(stderr, "unexpected engine exit: %s\n", message);
    abort();
}

__attribute__((weak)) void Shell_ExitSystemEx(
    const char *const log_message, const char *const dialog_message)
{
    Shell_ExitSystem(log_message);
}
