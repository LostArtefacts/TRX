#include "log.h"

#include "filesystem.h"
#include "memory.h"
#include "specific/s_log.h"

#include <stdarg.h>
#include <stdio.h>

FILE *m_LogHandle = NULL;

void Log_Init(void)
{
    char *full_path = File_GetFullPath("TR1X.log");
    m_LogHandle = fopen(full_path, "w");
    Memory_FreePointer(&full_path);

    S_Log_Init();
}

void Log_Message(
    const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    // print to log file
    if (m_LogHandle) {
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
    if (m_LogHandle) {
        fclose(m_LogHandle);
    }
}
