// enum_map.c reaches for exactly two symbols outside core: a formatting helper,
// and the localized game strings. Both live in translation units that drag in
// the engine's dependencies - strings/common.c wants PCRE2 - and neither is
// what the enum map is being tested for.
//
// Standing them up here keeps the unit tests engine-free, which is the whole
// reason they are a separate project (see meson.build).

#include <trx/core/memory.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// The real one cycles a ring of growing static buffers, so several results can
// be live at once; the caller never frees them. enum_map.c only keeps one key
// alive at a time, but match the contract rather than assume that.
//
// Weak, so a test that links core/strings itself - see lua/api/strings.c - gets
// the real thing instead of this.
#define M_BUFFER_COUNT 8

__attribute__((weak)) const char *String_FormatStaticV(
    const char *const fmt, va_list args)
{
    static char *m_Buffers[M_BUFFER_COUNT];
    static size_t m_Capacities[M_BUFFER_COUNT];
    static int32_t m_Next = 0;

    char **const buf = &m_Buffers[m_Next];
    size_t *const cap = &m_Capacities[m_Next];
    m_Next = (m_Next + 1) % M_BUFFER_COUNT;

    va_list probe;
    va_copy(probe, args);
    const int len = vsnprintf(nullptr, 0, fmt, probe);
    va_end(probe);

    const size_t needed = (size_t)len + 1;
    if (*cap < needed) {
        *buf = Memory_Realloc(*buf, needed);
        *cap = needed;
    }
    vsnprintf(*buf, *cap, fmt, args);
    return *buf;
}

__attribute__((weak)) const char *String_FormatStatic(
    const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const char *const result = String_FormatStaticV(fmt, args);
    va_end(args);
    return result;
}

// Enum labels are a UI concern. The name<->value reflection the Lua bridge and
// the docs rely on never touches them.
//
// Weak, so a test that is about the game strings themselves - see
// fakes/locale.c - can put a real table behind this instead.
__attribute__((weak)) const char *GameString_Get(const char *const key)
{
    return key;
}
