#include "log.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <ctype.h>
#include <pcre2.h>
#include <stdio.h>
#include <string.h>

bool String_EndsWith(const char *str, const char *suffix)
{
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return 0;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

bool String_Equivalent(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }

    size_t a_size = strlen(a);
    size_t b_size = strlen(b);
    if (a_size != b_size) {
        return false;
    }

    for (size_t i = 0; i < a_size; i++) {
        if (tolower(a[i]) != tolower(b[i])) {
            return false;
        }
    }
    return true;
}

const char *String_CaseSubstring(const char *subject, const char *pattern)
{
    if (subject == NULL || pattern == NULL) {
        return NULL;
    }

    size_t str_size = strlen(subject);
    size_t substr_size = strlen(pattern);
    if (substr_size > str_size) {
        return NULL;
    }
    if (substr_size == 0) {
        return subject;
    }

    for (size_t i = 0; i < str_size + 1 - substr_size; i++) {
        bool equivalent = true;
        for (size_t j = 0; j < substr_size; j++) {
            if (tolower(subject[i + j]) != tolower(pattern[j])) {
                equivalent = false;
                break;
            }
        }
        if (equivalent) {
            return subject + i;
        }
    }
    return NULL;
}

bool String_Match(const char *const subject, const char *const pattern)
{
    if (subject == NULL || pattern == NULL) {
        return 0;
    }

    const unsigned char *const usubject = (const unsigned char *)subject;
    const unsigned char *const upattern = (const unsigned char *)pattern;
    const uint32_t options = PCRE2_CASELESS;

    const uint32_t ovec_size = 128;

    int err_code;
    PCRE2_SIZE err_offset;
    pcre2_code *const re = pcre2_compile(
        upattern, PCRE2_ZERO_TERMINATED, options, &err_code, &err_offset, NULL);
    if (re == NULL) {
        PCRE2_UCHAR8 buffer[128];
        pcre2_get_error_message(err_code, buffer, 120);
        LOG_ERROR("%d\t%s", err_code, buffer);
        return false;
    }

    pcre2_match_data *const match_data =
        pcre2_match_data_create(ovec_size, NULL);
    const int rc = pcre2_match(
        re, usubject, PCRE2_ZERO_TERMINATED, 0,
        PCRE2_ANCHORED | PCRE2_ENDANCHORED, match_data, NULL);
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);

    return rc > 0;
}

bool String_IsEmpty(const char *const value)
{
    return String_Match(value, "^\\s*$");
}

bool String_ParseBool(const char *const value, bool *const target)
{
    if (String_Match(value, "0|false|off")) {
        if (target != NULL) {
            *target = false;
        }
        return true;
    }

    if (String_Match(value, "1|true|on")) {
        if (target != NULL) {
            *target = true;
        }
        return true;
    }

    return false;
}

bool String_ParseInteger(const char *const value, int32_t *const target)
{
    return sscanf(value, "%d", target) == 1;
}

bool String_ParseDecimal(const char *const value, float *const target)
{
    bool has_dot = false;
    for (size_t i = 0; i < strlen(value); i++) {
        if (i == 0 && value[i] == '-') {
            continue;
        }
        if (!isdigit(value[i])) {
            if (value[i] == '.') {
                if (has_dot) {
                    return false;
                }
                has_dot = true;
            } else {
                return false;
            }
        }
    }
    if (target != NULL) {
        *target = atof(value);
    }
    return true;
}

char *String_WordWrap(const char *text, const size_t line_len)
{
    if (text == NULL || line_len == 0) {
        return NULL;
    }

    const size_t text_len = strlen(text);
    char *const wrapped_text = Memory_Alloc(text_len + text_len / line_len + 2);

    size_t cur_line_len = 0;
    char *dest = wrapped_text;

    while (*text != '\0') {
        // Handle pre-existing newlines and leading spaces
        if (*text == '\n' || (cur_line_len == 0 && isspace(*text))) {
            if (*text == '\n') {
                cur_line_len = 0;
            }
            *dest++ = *text++;
            while (cur_line_len == 0 && isspace(*text) && *text != '\n') {
                text++;
            }
            continue;
        }

        // Find the length of the next word
        const char *end = text;
        while (*end != '\0' && !isspace(*end) && *end != '\n') {
            end++;
        }
        const size_t word_len = end - text;

        if (cur_line_len + word_len > line_len) {
            if (cur_line_len > 0) {
                *dest++ = '\n';
                cur_line_len = 0;
                continue;
            } else {
                for (size_t i = 0; i < word_len; i++) {
                    if (cur_line_len >= line_len) {
                        *dest++ = '\n';
                        cur_line_len = 0;
                    }
                    *dest++ = *text++;
                    cur_line_len++;
                }
                continue;
            }
        }

        // Copy the word to the destination
        for (size_t i = 0; i < word_len; i++) {
            *dest++ = *text++;
            cur_line_len++;
        }

        // Copy any spaces (handle overflow within the loop)
        while (*text != '\0' && isspace(*text) && *text != '\n') {
            if (cur_line_len >= line_len) {
                *dest++ = '\n';
                cur_line_len = 0;
                while (isspace(*text) && *text != '\n') {
                    text++;
                }
                break;
            }
            *dest++ = *text++;
            cur_line_len++;
        }
    }

    *dest = '\0';
    return wrapped_text;
}
