// Emscripten-specific log backtrace implementation.
// On the web platform, native backtraces are not available.

#include <trx/core/log.h>

bool Log_ShouldUseAnsiColors(void)
{
    // Browser console handles its own coloring; ANSI codes would be noise.
    return false;
}

void Log_Init_Extra(const char *path)
{
    (void)path;
}

void Log_Shutdown_Extra(void)
{
}

void Log_LogBacktrace(void)
{
}
