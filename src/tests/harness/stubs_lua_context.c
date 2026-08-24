#include <trx/game/lua/common.h>

static LUA_CONTEXT m_Context = LUA_CONTEXT_GLOBAL;

LUA_CONTEXT LUA_GetScriptContext(void)
{
    return m_Context;
}

void LUA_SetScriptContext(const LUA_CONTEXT context)
{
    m_Context = context;
}
