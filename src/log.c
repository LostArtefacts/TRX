#include "log.h"

#include "shared/memory.h"
#include "specific/s_log.h"

#include <stdarg.h>
#include <stdio.h>

FILE *m_LogHandle = NULL;

void Log_Init(const char *path)
{
    if (path != NULL) {
        m_LogHandle = fopen(path, "w");
    }
    S_Log_Init(path);
}

void Log_Message(
    const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    // print to log file
    if (m_LogHandle != NULL) {
        va_list vb;

        va_copy(vb, va);
        fprintf(m_LogHandle, "%s %d %s ", file, line, func);
        vfprintf(m_LogHandle, fmt, vb);
        fprintf(m_LogHandle, "\n");
        fflush(m_LogHandle);

        va_end(vb);
    }

    // print to stdout
    printf("%s %d %s ", file, line, func);
    vprintf(fmt, va);
    printf("\n");
    fflush(stdout);

    va_end(va);
}

void Log_Shutdown(void)
{
    S_Log_Shutdown();
    if (m_LogHandle != NULL) {
        fclose(m_LogHandle);
    }
}
