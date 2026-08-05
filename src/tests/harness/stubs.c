// Symbols every test needs regardless of which surface it exercises: the logger
// that ASSERT reaches for, and the settings several bridges read.
// Stubbing them here keeps the unit tests free of the platform log backend and
// gives the config a single definition across the whole test tree.

#include <trx/config/types.h>
#include <trx/core/log.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

CONFIG g_ConfigStorage = {};

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
