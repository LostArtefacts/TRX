#pragma once

#include <trx/core/colors.h>
#include <trx/core/vector.h>

#include <stdarg.h>
#include <stdint.h>

bool String_EndsWith(const char *str, const char *suffix);
bool String_Equivalent(const char *a, const char *b);

const char *String_CaseSubstring(const char *subject, const char *pattern);
bool String_Match(const char *subject, const char *pattern);

bool String_IsEmpty(const char *value);
bool String_ParseBool(const char *value, bool *target);
bool String_ParseInteger(const char *value, int32_t *target);
bool String_ParseDecimal(const char *value, float *target);
bool String_ParseRGB888(const char *value, RGB_888 *target);
bool String_ParseRGBA8888(const char *value, RGBA_8888 *target);

size_t String_GetCharByteSize(const char *ptr);
char *String_ToUpper(const char *text);
char *String_ToUpperPattern(const char *text);

// Rewrites text so that it can name a file: drops the characters a file system
// reserves, turns spaces into underscores, merges runs of them, and trims them
// from both ends. Returns an allocated string the caller frees.
char *String_ToFileName(const char *text);

// Wraps text to lines of at most `columns` characters. Existing line breaks
// stay in place, and words longer than `columns` are split at the limit.
// Returns an allocated string that the caller has to free.
char *String_Wrap(const char *text, int32_t columns);

VECTOR *String_Paginate(const char *text, int32_t max_lines);

typedef bool STRING_RANGE_FUNC(int32_t value, void *user_data);
char *String_FormatRanges(
    int32_t min_value, int32_t max_value, STRING_RANGE_FUNC predicate,
    void *user_data);

// ============================================================================

char *String_Format(const char *fmt, ...);

// Like String_Format, but prints into a specified string buffer.
// If the buffer is too small, reallocates it to fit the string.
void String_FormatInto(
    char **target_buf, size_t *target_cap, const char *fmt, ...);

// Like String_FormatInto, but accepts a va_list of arguments.
void String_FormatIntoV(
    char **target_buf, size_t *target_cap, const char *fmt, va_list args);

// Like String_Format, but writes into a static buffer that grows as needed.
// The caller must not free() the result; it will be freed on program exit.
const char *String_FormatStatic(const char *fmt, ...);

// Like String_FormatStatic, but accepts a va_list of arguments.
const char *String_FormatStaticV(const char *fmt, va_list args);
