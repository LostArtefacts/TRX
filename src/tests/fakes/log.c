// The log file, reduced to its last line. This one replaces the Log_Message
// stub in stubs.c, because where the line came from is exactly what is under
// test: the bridge walks the Lua stack to blame the caller, and one wrapper too
// many would blame the wrong line.

#include <fakes/log.h>

#include <harness/fake_calls.h>

#include <stdarg.h>
#include <stdio.h>

void Log_Message(
    const LOG_LEVEL level, const char *const file, const int32_t line,
    const char *const func, const char *const fmt, ...)
{
    char message[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    const char *const source = func != nullptr ? func : "?";
    FAKE_RECORD("log", FV(level), FV_STR(message), FV(line), FV_STR(source));
}
