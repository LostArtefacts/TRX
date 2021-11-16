#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

void T1MLogFunc(
    const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    printf("%s %d %s ", file, line, func);
    vprintf(fmt, va);
    printf("\n");
    va_end(va);
    fflush(stdout);
}
