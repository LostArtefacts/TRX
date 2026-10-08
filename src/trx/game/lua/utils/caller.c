#include <trx/game/lua/utils/caller.h>

#include <trx/game/lua/utils/args.h>

#include <lauxlib.h>
#include <string.h>

bool LUA_GetCallerInfo(lua_State *const L, lua_Debug *const ar)
{
    // Level 0 is the bridge. Above it sit the module's binding, a group's
    // __call, and strict mode's wrapper - all of them engine chunks.
    for (int32_t level = 1; lua_getstack(L, level, ar) != 0; level++) {
        if (lua_getinfo(L, "nSl", ar) == 0) {
            return false;
        }
        if (strncmp(
                ar->source, LUA_API_CHUNK_PREFIX,
                sizeof(LUA_API_CHUNK_PREFIX) - 1)
            != 0) {
            return true;
        }
    }
    return false;
}

void LUA_CheckLogCall(lua_State *const L, LUA_LOG_CALL *const out)
{
    *out = (LUA_LOG_CALL) {
        .level = LUA_CheckRange(L, 1, LOG_LEVEL_ERROR + 1, "unknown log level"),
        .msg = luaL_checkstring(L, 2),
        .src = "?",
        .func = "?",
        .line = 0,
    };
    if (LUA_GetCallerInfo(L, &out->ar)) {
        out->src = out->ar.short_src;
        out->func = out->ar.name != nullptr ? out->ar.name : "?";
        out->line = out->ar.currentline;
    }
}
