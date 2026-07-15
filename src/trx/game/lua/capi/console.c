#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <ctype.h>
#include <lauxlib.h>

static lua_State *m_L = nullptr;

// Command prefix -> Lua handler.
static const char m_HandlersKey[] = "trx.console.handlers";

// trxc.console.log(level, msg)
static int M_L_ConsoleLog(lua_State *const L)
{
    LUA_LOG_CALL call;
    LUA_CheckLogCall(L, &call);
    Console_LogEx(call.level, call.src, call.line, call.func, "%s", call.msg);
    return 0;
}

// trxc.console.clear()
static int M_L_ConsoleClear(lua_State *const L)
{
    Console_Clear();
    return 0;
}

// trxc.console.eval(cmd, { verbose = bool })
static int M_L_ConsoleEval(lua_State *const L)
{
    const char *cmd = luaL_checkstring(L, 1);
    bool verbose = false;
    if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
        lua_getfield(L, 2, "verbose");
        verbose = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    const bool old_verbose = Console_IsVerbose();
    Console_SetVerbose(verbose);
    COMMAND_RESULT res = Console_Eval(cmd);
    Console_SetVerbose(old_verbose);
    const char *err = "unknown error";
    switch (res) {
    case CR_BAD_INVOCATION:
        err = "bad invocation";
        break;
    case CR_UNAVAILABLE:
        err = "unavailable";
        break;
    case CR_FAILURE:
        err = "failure";
        break;
    case CR_SUCCESS:
        return 0;
    }
    return luaL_error(L, "console.eval %s: %s", err, cmd);
}

static COMMAND_RESULT M_LuaCommandProc(const COMMAND_CONTEXT *const ctx)
{
    lua_State *const L = m_L;
    if (L == nullptr) {
        return CR_FAILURE;
    }

    const int32_t base = lua_gettop(L);
    if (lua_getfield(L, LUA_REGISTRYINDEX, m_HandlersKey) != LUA_TTABLE) {
        lua_settop(L, base);
        return CR_FAILURE;
    }
    // The registered name, not ctx->prefix: the console matches without regard
    // to case, so what the player typed can differ.
    if (lua_getfield(L, -1, ctx->cmd->prefix) != LUA_TFUNCTION) {
        lua_settop(L, base);
        return CR_FAILURE;
    }

    lua_pushstring(L, ctx->args != nullptr ? ctx->args : "");
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        Console_LogError("%s: %s", ctx->prefix, lua_tostring(L, -1));
        lua_settop(L, base);
        return CR_FAILURE;
    }

    // Returning nothing is how a handler says it succeeded. Anything else has
    // to be a result the console knows: lua_tointeger reads a table or a string
    // as 0, which is CR_SUCCESS, so a handler that answered with nonsense would
    // report a clean run.
    COMMAND_RESULT result = CR_SUCCESS;
    if (!lua_isnil(L, -1)) {
        const bool is_result = lua_isinteger(L, -1)
            && ENUM_MAP_TO_STRING(COMMAND_RESULT, lua_tointeger(L, -1))
                != nullptr;
        if (!is_result) {
            Console_LogError(
                "%s: the handler must give back a console result, or nothing",
                ctx->prefix);
            lua_settop(L, base);
            return CR_FAILURE;
        }
        result = (COMMAND_RESULT)lua_tointeger(L, -1);
    }
    lua_settop(L, base);
    return result;
}

// trxc.console.register(name, help_id, fn)
static int M_L_ConsoleRegister(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    const char *const help_id = luaL_optstring(L, 2, nullptr);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    // A level script runs again every time its level is loaded.
    if (LUA_GetScriptContext() == LUA_CONTEXT_LEVEL) {
        return luaL_error(
            L,
            "console command '%s' cannot be registered from a level script: a "
            "command lives for the whole run",
            name);
    }

    // The name is interpolated into the dispatch regex in Console_Registry_Get,
    // so anything but a plain word would corrupt the matching of every command.
    if (name[0] == '\0') {
        return luaL_error(L, "console command name must not be empty");
    }
    for (const char *c = name; *c != '\0'; c++) {
        if (isalnum((unsigned char)*c) == 0 && *c != '_') {
            return luaL_error(
                L,
                "console command name '%s' must contain only letters, digits "
                "and underscores",
                name);
        }
    }

    if (Console_Registry_Get(name) != nullptr) {
        return luaL_error(
            L, "console command '%s' is already registered", name);
    }

    if (lua_getfield(L, LUA_REGISTRYINDEX, m_HandlersKey) != LUA_TTABLE) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, m_HandlersKey);
    }
    lua_pushvalue(L, 3);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);

    Console_Registry_Add((CONSOLE_COMMAND) {
        .prefix = name,
        .proc = M_LuaCommandProc,
        .help_id = help_id,
    });
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "log", M_L_ConsoleLog },     { "eval", M_L_ConsoleEval },
    { "clear", M_L_ConsoleClear }, { "register", M_L_ConsoleRegister },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    m_L = L;

    LUA_RegisterModule(L, "console", m_Module);
}

static void M_Shutdown(void)
{
    // The commands this state registered dispatch through M_LuaCommandProc and
    // their handlers live in the state that is about to close. Drop them so a
    // fresh state's scripts can register them again.
    Console_Registry_RemoveByProc(M_LuaCommandProc);
    m_L = nullptr;
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
