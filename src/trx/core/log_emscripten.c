// Emscripten-specific log backtrace implementation.
// On the web platform, native backtraces are not available.
// This provides a no-op implementation matching the interface
// expected by log.c for the "unknown" platform case.

#ifdef EMSCRIPTEN_BUILD

    #include <trx/core/log.h>

    #include <stdio.h>
    #include <string.h>

bool Log_ShouldUseAnsiColors(void)
{
    // Browser console handles its own coloring; ANSI codes would be noise.
    return false;
}

void Log_Init_Extra(const char *path)
{
    (void)path;
    // No extra log initialization needed on Emscripten.
}

void Log_Shutdown_Extra(void)
{
    // No extra log cleanup needed on Emscripten.
}

void Log_LogBacktrace(void)
{
    // Backtraces are not supported on the Emscripten/WebGL platform.
    // In debug builds, the browser's developer console provides stack traces
    // for JavaScript exceptions, which is the primary debugging mechanism.
}

#endif // EMSCRIPTEN_BUILD
