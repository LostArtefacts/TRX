#include <trx/game/input/names.h>

#include <stdio.h>

const char *Input_JoinKeyNames(
    const char *const *const names, const int32_t count)
{
    if (count <= 0) {
        return nullptr;
    }
    if (count == 1) {
        return names[0];
    }

    static char buf[256];
    buf[0] = '\0';
    size_t len = 0;
    for (int32_t i = 0; i < count; i++) {
        if (names[i] == nullptr) {
            continue;
        }
        const int32_t written = snprintf(
            buf + len, sizeof(buf) - len, "%s%s",
            len > 0 ? INPUT_COMBO_SEPARATOR : "", names[i]);
        if (written < 0) {
            break;
        }
        len += written;
        if (len >= sizeof(buf)) {
            len = sizeof(buf) - 1;
            break;
        }
    }
    return buf;
}
