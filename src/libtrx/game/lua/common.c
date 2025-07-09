#include "game/lua/common.h"

#include <debug.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

typedef struct {
    lua_State *state;
} M_PRIV;

static M_PRIV m_Priv = {};

void LUA_Init(void)
{
    M_PRIV *const p = &m_Priv;
    p->state = luaL_newstate();
    ASSERT(p->state != nullptr);
    luaL_openlibs(p->state);
}

void LUA_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    lua_close(p->state);
}
