// The only engine symbol the code under test reaches for is the logger, via
// ASSERT. Stubbing it keeps the unit tests free of the platform log backend,
// and therefore free of the engine entirely.

#include <trx/core/log.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

void Log_Message(
    const LOG_LEVEL level, const char *const file, const int32_t line,
    const char *const func, const char *const fmt, ...)
{
    (void)level;
    (void)file;
    (void)line;
    (void)func;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}
