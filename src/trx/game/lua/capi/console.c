#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <ctype.h>
#include <lauxlib.h>
#include <string.h>

static lua_State *m_L = nullptr;

// Command name -> Lua handler.
static const char m_HandlersKey[] = "trx.console.handlers";

// Command name -> Lua argument completer.
static const char m_CompletersKey[] = "trx.console.completers";

// trxc.console.log(level, msg)
static int M_L_ConsoleLog(lua_State *const L)
{
    LUA_LOG_CALL call;
    LUA_CheckLogCall(L, &call);
    Console_LogImpl(call.level, call.src, call.line, call.func, "%s", call.msg);
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
        Console_Error("%s: %s", ctx->prefix, lua_tostring(L, -1));
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
            Console_Error(
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

// The bridge a Lua command points its `complete` at: forwards the region past
// the command word to the script's completer, keyed by the command's canonical
// name, and fills `out` from what the parser suggests for `text` at `caret`.
// The span stays region-relative; the console's selector shifts it.
static void M_LuaArgComplete(
    const struct CONSOLE_COMMAND *const cmd, const char *const text,
    const int32_t caret, COMPLETION *const out)
{
    lua_State *const L = m_L;
    if (L == nullptr) {
        return;
    }

    const int32_t base = lua_gettop(L);
    if (lua_getfield(L, LUA_REGISTRYINDEX, m_CompletersKey) != LUA_TTABLE
        || lua_getfield(L, -1, cmd->prefix) != LUA_TFUNCTION) {
        lua_settop(L, base);
        return;
    }

    lua_pushstring(L, text != nullptr ? text : "");
    lua_pushinteger(L, caret);
    if (lua_pcall(L, 2, 3, 0) != LUA_OK) {
        Console_Error("%s: %s", cmd->prefix, lua_tostring(L, -1));
        lua_settop(L, base);
        return;
    }

    // Returns are suggestions, start, end.
    if (lua_isnumber(L, -2)) {
        out->start = (size_t)lua_tointeger(L, -2);
    }
    if (lua_isnumber(L, -1)) {
        out->end = (size_t)lua_tointeger(L, -1);
    }
    // The span is region-relative; keep it within the text the parser saw.
    const size_t region_len = text != nullptr ? strlen(text) : 0;
    if (out->end > region_len) {
        out->end = region_len;
    }
    if (out->start > out->end) {
        out->start = out->end;
    }
    if (lua_istable(L, -3)) {
        const int32_t count = (int32_t)lua_rawlen(L, -3);
        for (int32_t i = 1; i <= count; i++) {
            lua_rawgeti(L, -3, i);
            if (lua_isstring(L, -1)) {
                Completion_Add(out, lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_settop(L, base);
}

// Console_Registry_Get compares the name literally, so any spelling is safe to
// match; the alnum/underscore/dash rule keeps names typeable and completable.
// Raises on a bad or taken name.
static void M_CheckCommandName(lua_State *const L, const char *const name)
{
    if (name[0] == '\0') {
        luaL_error(L, "console command name must not be empty");
    }
    for (const char *c = name; *c != '\0'; c++) {
        if (isalnum((unsigned char)*c) == 0 && *c != '_' && *c != '-') {
            luaL_error(
                L,
                "console command name '%s' must contain only letters, digits, "
                "underscores and dashes",
                name);
        }
    }
    if (Console_Registry_Get(name) != nullptr) {
        luaL_error(L, "console command '%s' is already registered", name);
    }
}

// trxc.console.register(name, help_id, fn, [aliases], [complete_fn])
static int M_L_ConsoleRegister(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    const char *const help_id = luaL_optstring(L, 2, nullptr);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    const int32_t alias_count =
        lua_istable(L, 4) ? (int32_t)lua_rawlen(L, 4) : 0;
    const bool has_completer = lua_isfunction(L, 5);

    // A level script runs again every time its level is loaded.
    if (LUA_GetScriptContext() == LUA_CONTEXT_LEVEL) {
        return luaL_error(
            L,
            "console command '%s' cannot be registered from a level script: a "
            "command lives for the whole run",
            name);
    }

    // Validate every name and measure the display string before touching the
    // registry, so a bad alias leaves nothing half-registered.
    M_CheckCommandName(L, name);
    size_t joined_len = 0;
    for (int32_t i = 1; i <= alias_count; i++) {
        lua_rawgeti(L, 4, i);
        const char *const alias = luaL_checkstring(L, -1);
        M_CheckCommandName(L, alias);
        joined_len += strlen(alias) + (i > 1 ? 2 : 0);
        lua_pop(L, 1);
    }

    // The name reaches the handler; the aliases dispatch through the registry's
    // aliases field, so they share it without a lookup of their own.
    if (lua_getfield(L, LUA_REGISTRYINDEX, m_HandlersKey) != LUA_TTABLE) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, m_HandlersKey);
    }
    lua_pushvalue(L, 3);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);

    if (has_completer) {
        if (lua_getfield(L, LUA_REGISTRYINDEX, m_CompletersKey) != LUA_TTABLE) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, LUA_REGISTRYINDEX, m_CompletersKey);
        }
        lua_pushvalue(L, 5);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);
    }

    char *joined = nullptr;
    if (alias_count > 0) {
        joined = Memory_Alloc(joined_len + 1);
        joined[0] = '\0';
        for (int32_t i = 1; i <= alias_count; i++) {
            lua_rawgeti(L, 4, i);
            if (i > 1) {
                strcat(joined, ", ");
            }
            strcat(joined, lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }

    Console_Registry_Add((CONSOLE_COMMAND) {
        .prefix = name,
        .proc = M_LuaCommandProc,
        .complete = has_completer ? M_LuaArgComplete : nullptr,
        .help_id = help_id,
        .aliases = joined,
    });

    Memory_Free(joined);
    return 0;
}

// Pushes a command as { name, help, aliases }; help and aliases are left out
// when the command carries none.
static void M_PushCommand(lua_State *const L, const CONSOLE_COMMAND *const cmd)
{
    lua_newtable(L);
    lua_pushstring(L, cmd->prefix);
    lua_setfield(L, -2, "name");
    if (cmd->help_id != nullptr) {
        lua_pushstring(L, cmd->help_id);
        lua_setfield(L, -2, "help");
    }
    if (cmd->aliases != nullptr) {
        lua_pushstring(L, cmd->aliases);
        lua_setfield(L, -2, "aliases");
    }
}

// trxc.console.commands() -> a list of every command, each { name, help,
// aliases }, in registration order. help is the game string key, or absent;
// aliases is the comma-joined display string, or absent.
static int M_L_ConsoleCommands(lua_State *const L)
{
    VECTOR *const vec = Console_Registry_GetAll();
    const CONSOLE_COMMAND **const list =
        (const CONSOLE_COMMAND **)Vector_GetData(vec);
    lua_newtable(L);
    for (int32_t i = 0; i < vec->count; i++) {
        M_PushCommand(L, list[i]);
        lua_rawseti(L, -2, i + 1);
    }
    Vector_Free(vec);
    return 1;
}

// trxc.console.command(name) -> { name, help, aliases }, or nil. Matches by
// name or alias, as the console does when it dispatches.
static int M_L_ConsoleCommand(lua_State *const L)
{
    const CONSOLE_COMMAND *const cmd =
        Console_Registry_Get(luaL_checkstring(L, 1));
    if (cmd == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    M_PushCommand(L, cmd);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "log", M_L_ConsoleLog },
    { "eval", M_L_ConsoleEval },
    { "clear", M_L_ConsoleClear },
    { "register", M_L_ConsoleRegister },
    { "commands", M_L_ConsoleCommands },
    { "command", M_L_ConsoleCommand },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    m_L = L;

    LUA_RegisterModule(L, "console", m_Module);
}

static void M_Shutdown(void)
{
    // Every command in the registry came from this state, and its handler lives
    // in the state that is about to close. Drop them so a fresh state's scripts
    // can register them again.
    Console_Registry_Clear();
    m_L = nullptr;
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
