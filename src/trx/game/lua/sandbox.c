#include <trx/game/lua/sandbox.h>

#include <lauxlib.h>
#include <lualib.h>

// io and os hand a mod file and shell access. debug defeats the rest:
// debug.getupvalue on any trx.* function recovers the raw C bridge the module
// captured in a local.
void LUA_OpenSafeLibs(lua_State *const L)
{
    static const luaL_Reg libs[] = {
        { LUA_GNAME, luaopen_base },
        { LUA_LOADLIBNAME, luaopen_package },
        { LUA_COLIBNAME, luaopen_coroutine },
        { LUA_TABLIBNAME, luaopen_table },
        { LUA_STRLIBNAME, luaopen_string },
        { LUA_MATHLIBNAME, luaopen_math },
        { LUA_UTF8LIBNAME, luaopen_utf8 },
        { nullptr, nullptr },
    };
    for (const luaL_Reg *lib = libs; lib->func != nullptr; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);
    }
}

void LUA_HardenGlobals(lua_State *const L)
{
    lua_pushnil(L);
    lua_setglobal(L, "require");
    lua_pushnil(L);
    lua_setglobal(L, "package");

    // These come with the base library, not io/os: load() compiles bytecode
    // through an unhardened loader, dofile()/loadfile() read straight off disk.
    // Dropping io and os does not close them.
    lua_pushnil(L);
    lua_setglobal(L, "load");
    lua_pushnil(L);
    lua_setglobal(L, "loadfile");
    lua_pushnil(L);
    lua_setglobal(L, "dofile");

    // Serialises a function back to bytecode, feeding the loader above.
    lua_getglobal(L, "string");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        lua_setfield(L, -2, "dump");
    }
    lua_pop(L, 1);

    // getmetatable("") is shared by every string, and the string library leaves
    // it writable.
    lua_pushliteral(L, "");
    if (lua_getmetatable(L, -1) != 0) {
        lua_pushliteral(L, "locked");
        lua_setfield(L, -2, "__metatable");
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}
