// The console overlay, reduced to the last line written to it, plus the verbose
// flag - which is the only engine state trx.console.eval actually manipulates.

#include "fake_engine_console.h"

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

FAKE_CONSOLE_CALLS g_FakeConsoleCalls;
static bool m_Verbose;
static COMMAND_RESULT m_EvalResult;
static LUA_CONTEXT m_Context = LUA_CONTEXT_GLOBAL;

LUA_CONTEXT LUA_GetScriptContext(void)
{
    return m_Context;
}

void LUA_SetScriptContext(const LUA_CONTEXT context)
{
    m_Context = context;
}

void Console_LogEx(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
    g_FakeConsoleCalls.log_count++;
    g_FakeConsoleCalls.last_level = level;

    va_list args;
    va_start(args, fmt);
    vsnprintf(
        g_FakeConsoleCalls.last_message,
        sizeof(g_FakeConsoleCalls.last_message), fmt, args);
    va_end(args);
}

void Console_Clear(void)
{
    g_FakeConsoleCalls.clear_count++;
}

COMMAND_RESULT Console_Eval(const char *const cmdline)
{
    g_FakeConsoleCalls.eval_count++;
    snprintf(
        g_FakeConsoleCalls.last_command,
        sizeof(g_FakeConsoleCalls.last_command), "%s", cmdline);
    // The bridge sets verbose around the call and restores it afterwards, so
    // the value that matters is the one visible from in here.
    g_FakeConsoleCalls.verbose_during_eval = m_Verbose;
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

#define FAKE_MAX_COMMANDS 32
static CONSOLE_COMMAND m_Commands[FAKE_MAX_COMMANDS];
static int32_t m_CommandCount;

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

void Console_Registry_RemoveByProc(
    COMMAND_RESULT (*const proc)(const COMMAND_CONTEXT *ctx))
{
    int32_t kept = 0;
    for (int32_t i = 0; i < m_CommandCount; i++) {
        if (m_Commands[i].proc == proc) {
            free((char *)m_Commands[i].prefix);
            free((char *)m_Commands[i].help_id);
            free((char *)m_Commands[i].aliases);
            continue;
        }
        m_Commands[kept++] = m_Commands[i];
    }
    m_CommandCount = kept;
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

void FakeConsole_Reset(void)
{
    g_FakeConsoleCalls = (FAKE_CONSOLE_CALLS) {};
    m_Verbose = false;
    m_EvalResult = CR_SUCCESS;
    m_Context = LUA_CONTEXT_GLOBAL;
    // The commands stay: a command is registered once, at load time.
}

void FakeConsole_SetEvalResult(const COMMAND_RESULT result)
{
    m_EvalResult = result;
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
    m_Context = LUA_CONTEXT_LEVEL;
    const int status = lua_pcall(L, 0, 0, 0);
    m_Context = LUA_CONTEXT_GLOBAL;
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
// on a mod switch, and takes its commands with it. Every command a test
// registers dispatches through the one Lua proc, which is what the engine
// drops.
static int M_L_Reload(lua_State *const L)
{
    if (m_CommandCount > 0) {
        Console_Registry_RemoveByProc(m_Commands[0].proc);
    }
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

void FakeConsole_PushCalls(lua_State *const L)
{
    lua_pushinteger(L, g_FakeConsoleCalls.log_count);
    lua_setfield(L, -2, "log_count");
    lua_pushinteger(L, g_FakeConsoleCalls.last_level);
    lua_setfield(L, -2, "last_level");
    lua_pushstring(L, g_FakeConsoleCalls.last_message);
    lua_setfield(L, -2, "last_message");
    lua_pushinteger(L, g_FakeConsoleCalls.clear_count);
    lua_setfield(L, -2, "clear_count");
    lua_pushinteger(L, g_FakeConsoleCalls.eval_count);
    lua_setfield(L, -2, "eval_count");
    lua_pushstring(L, g_FakeConsoleCalls.last_command);
    lua_setfield(L, -2, "last_command");
    lua_pushboolean(L, g_FakeConsoleCalls.verbose_during_eval);
    lua_setfield(L, -2, "verbose_during_eval");
    lua_pushboolean(L, Console_IsVerbose());
    lua_setfield(L, -2, "verbose_now");
}
