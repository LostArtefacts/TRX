#include <trx/game/game_strings/lang_match.h>

#include <trx/core/strings.h>

#include <ctype.h>
#include <string.h>

typedef enum {
    M_EXACT,
    M_BASE,
    M_SIBLING,
} M_PASS;

static size_t M_GetBaseLength(const char *const code)
{
    const char *const sep = strchr(code, '-');
    return sep != nullptr ? (size_t)(sep - code) : strlen(code);
}

static bool M_HasSameBase(const char *const a, const char *const b)
{
    const size_t len = M_GetBaseLength(a);
    if (len != M_GetBaseLength(b)) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) {
            return false;
        }
    }
    return true;
}

static bool M_Accepts(
    const M_PASS pass, const char *const available, const char *const wanted)
{
    switch (pass) {
    case M_EXACT:
        return String_Equivalent(available, wanted);
    case M_BASE:
        return M_GetBaseLength(available) == strlen(available)
            && M_HasSameBase(available, wanted);
    case M_SIBLING:
        return M_HasSameBase(available, wanted);
    }
    return false;
}

const char *GameStringLang_MatchPreferred(
    const VECTOR *const available, const VECTOR *const preferred)
{
    if (available == nullptr || preferred == nullptr) {
        return nullptr;
    }
    for (M_PASS pass = M_EXACT; pass <= M_SIBLING; pass++) {
        for (int32_t i = 0; i < preferred->count; i++) {
            const char *const wanted = *(char **)Vector_Get(preferred, i);
            if (wanted == nullptr || *wanted == '\0') {
                continue;
            }
            for (int32_t j = 0; j < available->count; j++) {
                const char *const code = *(char **)Vector_Get(available, j);
                if (code != nullptr && M_Accepts(pass, code, wanted)) {
                    return code;
                }
            }
        }
    }
    return nullptr;
}
