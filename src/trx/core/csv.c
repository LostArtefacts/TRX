#include <trx/core/csv.h>

#include <trx/debug.h>

#include <ctype.h>
#include <string.h>

const char *CSV_SkipWhitespace(const char *value)
{
    ASSERT(value != nullptr);

    while (*value != '\0' && isspace((unsigned char)*value)) {
        value++;
    }
    return value;
}

char *CSV_Trim(char *value)
{
    ASSERT(value != nullptr);

    value = (char *)CSV_SkipWhitespace(value);
    if (*value == '\0') {
        return value;
    }

    char *end = value + strlen(value) - 1;
    while (end > value && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return value;
}

void CSV_ParseField(
    const char **const cursor, char *const out, const size_t out_size)
{
    ASSERT(cursor != nullptr);
    ASSERT(*cursor != nullptr);
    ASSERT(out != nullptr);
    ASSERT(out_size > 0);

    const char *src = *cursor;
    char *dst = out;
    char *const end = out + out_size - 1;

    if (*src == '"') {
        src++;
        while (*src != '\0' && (*src != '"' || src[1] == '"')) {
            if (*src == '"' && src[1] == '"') {
                src++;
            }
            if (dst < end) {
                *dst = *src;
                dst++;
            }
            src++;
        }
        if (*src == '"') {
            src++;
        }
    } else {
        while (*src != '\0' && *src != ',') {
            if (dst < end) {
                *dst = *src;
                dst++;
            }
            src++;
        }
    }

    *dst = '\0';
    if (*src == ',') {
        src++;
    }
    *cursor = src;
}
