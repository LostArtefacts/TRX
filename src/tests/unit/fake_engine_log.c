// The log file, reduced to its last line. This one replaces the Log_Message
// stub in stubs.c, because where the line came from is exactly what is under
// test: the bridge walks the Lua stack to blame the caller, and one wrapper too
// many would blame the wrong line.

#include "fake_engine_log.h"

#include <stdarg.h>
#include <stdio.h>

FAKE_LOG_CALLS g_FakeLogCalls;

void Log_Message(
    const LOG_LEVEL level, const char *const file, const int32_t line,
    const char *const func, const char *const fmt, ...)
{
    g_FakeLogCalls.count++;
    g_FakeLogCalls.last_level = level;
    g_FakeLogCalls.last_line = line;
    snprintf(
        g_FakeLogCalls.last_func, sizeof(g_FakeLogCalls.last_func), "%s",
        func != nullptr ? func : "?");

    va_list args;
    va_start(args, fmt);
    vsnprintf(
        g_FakeLogCalls.last_message, sizeof(g_FakeLogCalls.last_message), fmt,
        args);
    va_end(args);
}

void FakeLog_Reset(void)
{
    g_FakeLogCalls = (FAKE_LOG_CALLS) {};
}
