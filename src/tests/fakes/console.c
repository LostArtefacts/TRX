// The console overlay, reduced to the last line written to it, plus the verbose
// flag - which is the only engine state trx.console.eval manipulates.

#include <fakes/console.h>

#include <harness/fake_calls.h>

#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/console/types.h>
#include <trx/game/lua/common.h>

#include <assert.h>
#include <lauxlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define FAKE_MAX_COMMANDS 32
static CONSOLE_COMMAND m_Commands[FAKE_MAX_COMMANDS];
static int32_t m_CommandCount;

static bool m_Verbose;
static COMMAND_RESULT m_EvalResult;
// The aliases arrive comma-joined ("secondary, third"); each spelling
// dispatches.
static bool M_FakeAliasMatch(const char *const aliases, const char *const word)
{
    if (aliases == nullptr) {
        return false;
    }
    const size_t word_len = strlen(word);
    const char *p = aliases;
    while (*p != '\0') {
        while (*p == ',' || *p == ' ') {
            p++;
        }
        const char *const start = p;
        while (*p != '\0' && *p != ',') {
            p++;
        }
        const char *end = p;
        while (end > start && end[-1] == ' ') {
            end--;
        }
        if ((size_t)(end - start) == word_len
            && strncasecmp(start, word, word_len) == 0) {
            return true;
        }
    }
    return false;
}

static void M_Reset(void)
{
    m_Verbose = false;
    m_EvalResult = CR_SUCCESS;
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    // The commands stay: a command is registered once, at load time.
}

// fake.set_eval_result(result) - what the next Console_Eval hands back.
static int M_L_SetEvalResult(lua_State *const L)
{
    FakeConsole_SetEvalResult((COMMAND_RESULT)luaL_checkinteger(L, 1));
    return 0;
}

// fake.run(prefix, args) - the console running a command the player typed.
static int M_L_Run(lua_State *const L)
{
    lua_pushinteger(
        L, FakeConsole_Run(luaL_checkstring(L, 1), luaL_optstring(L, 2, "")));
    return 1;
}

// fake.as_level_script(fn) - run fn the way the engine runs a level script.
static int M_L_AsLevelScript(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);
    const int status = lua_pcall(L, 0, 0, 0);
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    if (status != LUA_OK) {
        return lua_error(L);
    }
    return 0;
}

static int M_L_IsRegistered(lua_State *const L)
{
    lua_pushboolean(L, Console_Registry_Get(luaL_checkstring(L, 1)) != nullptr);
    return 1;
}

// fake.reload() - the state that registered the commands shuts down, as it does
// on a mod switch, and takes its commands with it.
static int M_L_Reload(lua_State *const L)
{
    Console_Registry_Clear();
    return 0;
}

// fake.complete_args(cmd, text[, caret]) - the suggestions the registered
// argument suggester gives back as a list, and the region-relative byte offsets
// start, end of the run they replace.
static int M_L_CompleteArgs(lua_State *const L)
{
    const char *const cmd = luaL_checkstring(L, 1);
    const char *const text = luaL_optstring(L, 2, "");
    const int32_t caret =
        (int32_t)luaL_optinteger(L, 3, (lua_Integer)strlen(text));
    lua_newtable(L);
    const CONSOLE_COMMAND *const command = Console_Registry_Get(cmd);
    if (command == nullptr || command->complete == nullptr) {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        return 3;
    }
    COMPLETION out;
    Completion_Init(&out);
    command->complete(command, text, caret, &out);
    for (int32_t i = 0; i < out.suggestions->count; i++) {
        const SUGGESTION *const s = Vector_Get(out.suggestions, i);
        lua_pushstring(L, s->text);
        lua_rawseti(L, -2, i + 1);
    }
    lua_pushinteger(L, (lua_Integer)out.start);
    lua_pushinteger(L, (lua_Integer)out.end);
    Completion_Free(&out);
    return 3;
}

static int M_L_HelpId(lua_State *const L)
{
    const char *const help_id = FakeConsole_HelpId(luaL_checkstring(L, 1));
    if (help_id == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, help_id);
    }
    return 1;
}

// fake.is_verbose() - the console's own flag, which is state rather than
// something the console was asked to do.
static int M_L_IsVerbose(lua_State *const L)
{
    lua_pushboolean(L, Console_IsVerbose());
    return 1;
}

void Console_LogImpl(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
    char message[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    FAKE_RECORD("log", FV(level), FV_STR(message));
}

void Console_Clear(void)
{
    FAKE_RECORD("clear");
}

COMMAND_RESULT Console_Eval(const char *const cmdline)
{
    // The bridge sets verbose around the call and restores it afterwards, so
    // the value that matters is the one visible from in here.
    const bool verbose = m_Verbose;
    FAKE_RECORD("eval", FV_STR(cmdline), FV(verbose));
    return m_EvalResult;
}

void Console_SetVerbose(const bool verbose)
{
    m_Verbose = verbose;
}

bool Console_IsVerbose(void)
{
    return m_Verbose;
}

void Console_Registry_Add(const CONSOLE_COMMAND command)
{
    // Dropping one would leave the test that registered it looking at a command
    // that is not there, and passing.
    assert(m_CommandCount < FAKE_MAX_COMMANDS);
    CONSOLE_COMMAND *const slot = &m_Commands[m_CommandCount++];
    *slot = command;
    // Copied, as the real registry copies them: a Lua registration's strings
    // belong to the Lua state.
    slot->prefix = command.prefix != nullptr ? strdup(command.prefix) : nullptr;
    slot->help_id =
        command.help_id != nullptr ? strdup(command.help_id) : nullptr;
    slot->aliases =
        command.aliases != nullptr ? strdup(command.aliases) : nullptr;
}

// The real registry matches a command name case-insensitively, so a test that
// matched exactly would not see what the player typing /HEAL sees.
const CONSOLE_COMMAND *Console_Registry_Get(const char *const prefix)
{
    for (int32_t i = 0; i < m_CommandCount; i++) {
        if (strcasecmp(m_Commands[i].prefix, prefix) == 0
            || M_FakeAliasMatch(m_Commands[i].aliases, prefix)) {
            return &m_Commands[i];
        }
    }
    return nullptr;
}

VECTOR *Console_Registry_GetAll(void)
{
    VECTOR *const vec = Vector_Create(sizeof(const CONSOLE_COMMAND *));
    for (int32_t i = 0; i < m_CommandCount; i++) {
        const CONSOLE_COMMAND *const cmd = &m_Commands[i];
        Vector_Add(vec, &cmd);
    }
    return vec;
}

void Console_Registry_Clear(void)
{
    for (int32_t i = 0; i < m_CommandCount; i++) {
        free((char *)m_Commands[i].prefix);
        free((char *)m_Commands[i].help_id);
        free((char *)m_Commands[i].aliases);
    }
    m_CommandCount = 0;
}

COMMAND_RESULT FakeConsole_Run(const char *const prefix, const char *const args)
{
    const CONSOLE_COMMAND *const command = Console_Registry_Get(prefix);
    if (command == nullptr) {
        return CR_BAD_INVOCATION;
    }
    const COMMAND_CONTEXT ctx = {
        .cmd = command,
        .prefix = prefix,
        .args = args,
    };
    return command->proc(&ctx);
}

int32_t FakeConsole_CommandCount(void)
{
    return m_CommandCount;
}

const char *FakeConsole_HelpId(const char *const prefix)
{
    const CONSOLE_COMMAND *const command = Console_Registry_Get(prefix);
    return command != nullptr ? command->help_id : nullptr;
}

FAKE_ON_RESET(M_Reset)

void FakeConsole_SetEvalResult(const COMMAND_RESULT result)
{
    m_EvalResult = result;
}

void FakeConsole_PushLua(lua_State *const L)
{
    lua_pushcfunction(L, M_L_SetEvalResult);
    lua_setfield(L, -2, "set_eval_result");
    lua_pushcfunction(L, M_L_Run);
    lua_setfield(L, -2, "run");
    lua_pushcfunction(L, M_L_HelpId);
    lua_setfield(L, -2, "help_id");
    lua_pushcfunction(L, M_L_CompleteArgs);
    lua_setfield(L, -2, "complete_args");
    lua_pushcfunction(L, M_L_IsRegistered);
    lua_setfield(L, -2, "is_registered");
    lua_pushcfunction(L, M_L_AsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_L_Reload);
    lua_setfield(L, -2, "reload");
    lua_pushcfunction(L, M_L_IsVerbose);
    lua_setfield(L, -2, "is_verbose");

    lua_newtable(L);
    lua_pushinteger(L, CR_SUCCESS);
    lua_setfield(L, -2, "SUCCESS");
    lua_pushinteger(L, CR_FAILURE);
    lua_setfield(L, -2, "FAILURE");
    lua_pushinteger(L, CR_UNAVAILABLE);
    lua_setfield(L, -2, "UNAVAILABLE");
    lua_pushinteger(L, CR_BAD_INVOCATION);
    lua_setfield(L, -2, "BAD_INVOCATION");
    lua_setfield(L, -2, "CommandResult");
}
