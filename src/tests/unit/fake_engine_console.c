// The console overlay, reduced to the last line written to it, plus the verbose
// flag - which is the only engine state trx.console.eval actually manipulates.

#include "fake_engine_console.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

FAKE_CONSOLE_CALLS g_FakeConsoleCalls;
static bool m_Verbose;
static COMMAND_RESULT m_EvalResult;

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

void FakeConsole_Reset(void)
{
    g_FakeConsoleCalls = (FAKE_CONSOLE_CALLS) {};
    m_Verbose = false;
    m_EvalResult = CR_SUCCESS;
}

void FakeConsole_SetEvalResult(const COMMAND_RESULT result)
{
    m_EvalResult = result;
}
