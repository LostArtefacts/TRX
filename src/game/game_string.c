#include "game_string.h"

#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <assert.h>
#include <stddef.h>
#include <string.h>

typedef struct ENUM_NAME_MAP {
    const char *str;
    const GAME_STRING_ID val;
} ENUM_NAME_MAP;

static ENUM_NAME_MAP m_EnumNameMap[];
static char *m_StringMap[GS_NUMBER_OF] = { 0 };
#undef GS_DEFINE
#define GS_DEFINE(id, str) str,
static const char *m_DefaultStringMap[GS_NUMBER_OF] = {
#include "game/game_string.def"
};

#undef GS_DEFINE
#define GS_DEFINE(id, str)                                                     \
    {                                                                          \
        QUOTE(id),                                                             \
        GS_##id,                                                               \
    },
static ENUM_NAME_MAP m_EnumNameMap[] = {
#include "game/game_string.def"
    { NULL, 0 },
};

void GameString_Set(GAME_STRING_ID id, const char *value)
{
    assert(id >= 0);
    assert(id < GS_NUMBER_OF);
    Memory_FreePointer(&m_StringMap[id]);
    m_StringMap[id] = Memory_DupStr(value);
}

const char *GameString_Get(GAME_STRING_ID id)
{
    return m_StringMap[id] != NULL ? (const char *)m_StringMap[id]
                                   : m_DefaultStringMap[id];
}

GAME_STRING_ID GameString_IDFromEnum(const char *const str)
{
    const ENUM_NAME_MAP *current = &m_EnumNameMap[0];
    while (current->str) {
        if (!strcmp(str, current->str)) {
            return current->val;
        }
        current++;
    }
    return GS_INVALID;
}
