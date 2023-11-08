#include "strings.h"

#include <ctype.h>
#include <string.h>

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

const char *String_CaseSubstring(const char *str, const char *substr)
{
    if (str == NULL || substr == NULL) {
        return NULL;
    }

    size_t str_size = strlen(str);
    size_t substr_size = strlen(substr);
    if (substr_size > str_size) {
        return NULL;
    }
    if (substr_size == 0) {
        return str;
    }

    for (size_t i = 0; i < str_size + 1 - substr_size; i++) {
        bool equivalent = true;
        for (size_t j = 0; j < substr_size; j++) {
            if (tolower(str[i + j]) != tolower(substr[j])) {
                equivalent = false;
                break;
            }
        }
        if (equivalent) {
            return str + i;
        }
    }
    return NULL;
}
