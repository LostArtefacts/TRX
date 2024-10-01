#include "specific/s_flagged_string.h"

#include "global/types.h"

void __thiscall S_FlaggedString_Create(STRING_FLAGGED *string, int32_t size)
{
    string->content = malloc(size);
    if (string->content != NULL) {
        *string->content = '\0';
        string->is_valid = true;
    }
}

void __thiscall S_FlaggedString_InitAdapter(DISPLAY_ADAPTER *adapter)
{
    S_FlaggedString_Create(&adapter->driver_desc, 256);
    S_FlaggedString_Create(&adapter->driver_name, 256);
}

void __thiscall S_FlaggedString_Delete(STRING_FLAGGED *string)
{
    if (string->is_valid && string->content) {
        free(string->content);
        string->content = NULL;
        string->is_valid = false;
    }
}

bool S_FlaggedString_Copy(STRING_FLAGGED *dst, STRING_FLAGGED *src)
{
    if (dst == NULL || src == NULL || dst == src || !src->is_valid) {
        return false;
    }

    size_t src_len = lstrlen(src->content);
    dst->is_valid = false;
    dst->content = malloc(src_len + 1);
    if (dst->content == NULL) {
        return false;
    }

    if (src_len > 0) {
        lstrcpy(dst->content, src->content);
    } else {
        *dst->content = 0;
    }
    dst->is_valid = true;
    return true;
}
