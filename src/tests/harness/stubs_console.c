// The console, for a test that links a module which reports through it but is
// not testing what the console does with the report. Weak, so that a test
// with a console fake of its own keeps that one.

#include <trx/game/console/common.h>

__attribute__((weak)) void Console_LogImpl(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
}

__attribute__((weak)) void Console_ShowImpl(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
}
