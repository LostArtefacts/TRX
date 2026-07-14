// The console overlay, reduced to the last line written to it, plus the verbose
// flag - which is the only engine state trx.console.eval actually manipulates.

#include "fake_engine_console.h"

#include <trx/game/console/registry.h>
#include <trx/game/console/types.h>
#include <trx/game/lua/common.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
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
}

// The real registry matches a command with a case-insensitive regex, so a test
// that matched exactly would not see what the player typing /HEAL sees.
const CONSOLE_COMMAND *Console_Registry_Get(const char *const prefix)
{
    for (int32_t i = 0; i < m_CommandCount; i++) {
        if (strcasecmp(m_Commands[i].prefix, prefix) == 0) {
            return &m_Commands[i];
        }
    }
    return nullptr;
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
